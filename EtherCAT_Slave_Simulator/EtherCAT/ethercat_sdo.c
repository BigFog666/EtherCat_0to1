#include "ethercat_sdo.h"

bool EC_Mailbox_Send(EC_Mailbox *mailbox, const EC_SDO_Request *request)
{
    if (mailbox == 0 || request == 0) {
        return false;
    }
    /* 上一笔事务没结束时，不接受新请求，避免数据被覆盖。 */
    if (mailbox->state != EC_MAILBOX_EMPTY) {
        return false;
    }
    mailbox->request = *request; /* 复制一份请求，而不是保留调用者的指针。 */
    mailbox->state = EC_MAILBOX_REQUEST_READY;
    return true;
}

bool EC_Mailbox_Process(EC_Mailbox *mailbox, const EC_ObjectDictionary *od)
{
    if (mailbox == 0 || od == 0) {
        return false;
    }
    if (mailbox->state != EC_MAILBOX_REQUEST_READY) {
        return false; /* 没有待处理请求时，不做字典读写。 */
    }

    /* 先准备新的响应，防止上一笔读取的数值残留。
     * 请求中的索引和子索引被带回，主站能知道回应的是哪个对象。 */
    const EC_SDO_Request *request = &mailbox->request;
    mailbox->response = (EC_SDO_Response){
        .index = request->index,
        .subindex = request->subindex,
        .result = EC_OD_INVALID_ARGUMENT,
        .value = 0
    };

    /* 本课最重要的两条路径：读请求交给 OD_Read，写请求交给 OD_Write。
     * SDO 不重新实现查表和权限检查，而是复用第七课的对象字典。 */
    switch (request->service) {
    case EC_SDO_UPLOAD:
        mailbox->response.result = EC_OD_ReadI32(
            od, request->index, request->subindex, &mailbox->response.value);
        break;
    case EC_SDO_DOWNLOAD:
        mailbox->response.result = EC_OD_WriteI32(
            od, request->index, request->subindex, request->value);
        break;
    default:
        /* 不认识的服务返回失败响应，不修改任何对象。 */
        break;
    }

    /* 对象不存在或写入被拒绝，也需要把失败响应返回给主站。 */
    mailbox->state = EC_MAILBOX_RESPONSE_READY;
    return true;
}

bool EC_Mailbox_Receive(EC_Mailbox *mailbox, EC_SDO_Response *response)
{
    if (mailbox == 0 || response == 0) {
        return false;
    }
    if (mailbox->state != EC_MAILBOX_RESPONSE_READY) {
        return false;
    }
    *response = mailbox->response; /* 将响应交给调用者。 */
    mailbox->state = EC_MAILBOX_EMPTY; /* 这笔事务结束，允许下一笔请求。 */
    return true;
}

bool EC_SDO_Transfer(EC_Mailbox *mailbox, const EC_ObjectDictionary *od,
                     const EC_SDO_Request *request, EC_SDO_Response *response)
{
    /* 先确认参数完整，避免发送后才发现没有字典或响应接收变量。 */
    if (mailbox == 0 || od == 0 || request == 0 || response == 0) {
        return false;
    }
    if (!EC_Mailbox_Send(mailbox, request)) {
        return false;
    }
    if (!EC_Mailbox_Process(mailbox, od)) {
        return false;
    }
    return EC_Mailbox_Receive(mailbox, response);
}
