#ifndef ETHERCAT_SDO_H
#define ETHERCAT_SDO_H

#include <stdbool.h>
#include "ethercat_od.h"

/* 第八课第一步：模拟已经解析好的 SDO 请求和响应。
 * 这些 C 结构体不是实际 CoE/SDO 报文，不能直接发送到网卡。
 * 本课先只支持 int32_t，后续再加入报文字节格式和标准 Abort Code。 */
typedef enum {
    EC_SDO_UPLOAD,  /* 从站把对象值上传给主站：主站读取。 */
    EC_SDO_DOWNLOAD /* 主站把数值下载到从站：主站写入。 */
} EC_SDO_Service;

typedef struct {
    EC_SDO_Service service; /* 请求读还是写。 */
    uint16_t index;         /* 访问哪个对象，例如 0x607A。 */
    uint8_t subindex;       /* 访问哪个子索引，本课用 0。 */
    int32_t value;          /* Download 要写入的值；Upload 不使用此字段。 */
} EC_SDO_Request;

typedef struct {
    uint16_t index;      /* 回应的是哪个对象。 */
    uint8_t subindex;
    EC_OD_Result result; /* 复用字典结果表示成功/失败，尚不是标准 SDO 错误码。 */
    int32_t value;       /* 仅在 Upload 成功时表示读到的值。 */
} EC_SDO_Response;

/* 单次只容纳一笔事务，避免覆盖还没处理的请求或还没取走的响应。
 * 这是教学中的邮箱状态，与 INIT/PRE-OP 等通信状态不是同一个状态机。 */
typedef enum {
    EC_MAILBOX_EMPTY = 0,
    EC_MAILBOX_REQUEST_READY,
    EC_MAILBOX_RESPONSE_READY
} EC_Mailbox_State;

typedef struct {
    EC_Mailbox_State state;
    EC_SDO_Request request;
    EC_SDO_Response response;
} EC_Mailbox;

/* 使用前将邮箱初始化为 {0}，此时状态为 EMPTY。
 * Send 放入请求，Process 模拟从站处理，Receive 取回响应并清空状态。
 * 本模型单线程运行，不包含真实 ESC/SyncManager 的邮箱管理。 */
bool EC_Mailbox_Send(EC_Mailbox *mailbox, const EC_SDO_Request *request);
bool EC_Mailbox_Process(EC_Mailbox *mailbox, const EC_ObjectDictionary *od);
bool EC_Mailbox_Receive(EC_Mailbox *mailbox, EC_SDO_Response *response);

/* 便于第一遍学习：依次完成发送、处理、接收。
 * 返回 true 只表示拿到了响应；对象读写是否成功还要检查 response.result。
 * 真实主站和从站各自运行、通过网络交换消息，不会直接这样相互调用。 */
bool EC_SDO_Transfer(EC_Mailbox *mailbox, const EC_ObjectDictionary *od,
                     const EC_SDO_Request *request, EC_SDO_Response *response);

#endif
