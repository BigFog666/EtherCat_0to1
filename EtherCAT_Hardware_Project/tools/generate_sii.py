"""按项目 ESI 生成最小 SII；只写本地文件，不访问网卡或 EEPROM。"""
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def integer(text):
    return int(text.replace('#x', '0x'), 0)


def crc8(data):
    crc = 0xff
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ (7 if crc & 0x80 else 0)) & 0xff
    return crc


def category(kind, payload):
    if len(payload) & 1:
        payload += b'\0'
    return struct.pack('<HH', kind, len(payload)//2) + payload


def generate():
    tree = ET.parse(ROOT / 'config/EtherCAT_Joint_F407.xml')
    device = tree.find('.//Device')
    size = integer(device.findtext('Eeprom/ByteSize'))
    header = bytearray(128)
    config = bytes.fromhex(device.findtext('Eeprom/ConfigData'))
    if len(config) != 14:
        raise ValueError('本项目 ConfigData 必须正好 14 字节')
    header[:14] = config
    struct.pack_into('<H', header, 14, crc8(config))
    dtype = device.find('Type')
    identity = [integer(tree.findtext('Vendor/Id')), integer(dtype.get('ProductCode')),
                integer(dtype.get('RevisionNo')), 0]
    struct.pack_into('<4I', header, 16, *identity)
    sms = device.findall('Sm')
    values = [(integer(sm.get('StartAddress')), integer(sm.get('DefaultSize')),
               integer(sm.get('ControlByte')), integer(sm.get('Enable'))) for sm in sms]
    struct.pack_into('<5H', header, 48, values[0][0], values[0][1], values[1][0], values[1][1], 4)
    struct.pack_into('<2H', header, 124, size//128-1, 1)
    strings = [device.findtext('GroupType'), dtype.text, device.findtext('Name')]
    payload = bytes([len(strings)])
    for s in strings:
        encoded = s.encode('ascii')
        payload += bytes([len(encoded)]) + encoded
    general = bytearray(32)
    general[0], general[2], general[3], general[5], general[11] = 1, 2, 3, 3, 4
    general[14] = 1
    struct.pack_into('<H', general, 16, 0x11)  # LAN9252 两个 Ethernet PHY 端口。
    sm_payload = b''.join(struct.pack('<HHBBBB', address, length, control, 0, enabled, i+1)
                          for i, (address, length, control, enabled) in enumerate(values))
    data = bytes(header) + category(10, payload) + category(30, bytes(general))
    data += category(40, b'\x01\x02\x03\x00') + category(41, sm_payload) + b'\xff\xff'
    # CoE 设备的 PDO 从对象字典读取；不重复编码 PDO/DC 类别。
    if len(data) > size:
        raise ValueError('SII 超出 ESI 声明容量')
    return data.ljust(size, b'\xff')


def main():
    output = ROOT / 'build/config/EtherCAT_Joint_F407.sii.bin'
    data = generate()
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_bytes() != data:
        output.write_bytes(data)
    print('Generated local SII:', output, 'bytes=', len(data), '(not hardware validated)')


if __name__ == '__main__':
    main()
