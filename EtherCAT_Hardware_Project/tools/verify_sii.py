"""检查 EEPROM 备份基本完整性；项目读回必须逐字节匹配生成镜像。"""
from pathlib import Path
import argparse
import struct
from generate_sii import crc8, generate
parser = argparse.ArgumentParser()
parser.add_argument('file', type=Path)
parser.add_argument('--project', action='store_true')
args = parser.parse_args()
data = args.file.read_bytes()
if len(data) < 128 or len(data) != (struct.unpack_from('<H', data, 124)[0]+1)*128:
    raise SystemExit('FAIL: EEPROM 备份长度不符')
if data[14] != crc8(data[:14]):
    raise SystemExit('FAIL: EEPROM 头 CRC 不符')
if args.project and data != generate():
    raise SystemExit('FAIL: 写后读回与项目 SII 不同')
print('PASS: SII length/CRC', 'and exact project readback' if args.project else '(not proof of factory ownership)', len(data))
