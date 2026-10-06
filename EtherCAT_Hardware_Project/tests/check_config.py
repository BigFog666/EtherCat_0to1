"""交叉核对 ESI/SII/SSC 映射与镜像范围，不代替硬件或 ETG 一致性测试。"""
from pathlib import Path
import struct
import sys
import re
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from generate_sii import generate, crc8

def check(condition, message):
    if not condition:
        raise AssertionError(message)

tree = ET.parse(ROOT / 'config/EtherCAT_Joint_F407.xml')
device = tree.find('.//Device')
header = (ROOT / 'firmware/generated/cia402appl.h').read_text(encoding='utf-8')
for pdo in ['RxPdo', 'TxPdo']:
    entries = device.find(pdo).findall('Entry')
    check(sum(int(e.findtext('BitLen')) for e in entries) == 96, pdo + ' size')
    for entry in entries:
        index = int(entry.findtext('Index').replace('#x', '0x'), 0)
        sub = int(entry.findtext('SubIndex'))
        bits = int(entry.findtext('BitLen'))
        check(('0x%08X' % ((index << 16) | (sub << 8) | bits)).lower() in header.lower(), pdo + ' mapping')
data = generate()
check(len(data) == 2048, 'SII size')
check(data[14] == crc8(data[:14]) and data[15] == 0, 'SII CRC')
check(struct.unpack_from('<3I', data, 16) == (0xfdfff, 0x26483052, 0x10000), 'SII identity')
identity_header = (ROOT / 'common/project_identity.h').read_text(encoding='utf-8')
ssc_config = (ROOT / 'firmware/generated/ecat_def.h').read_text(encoding='utf-8')
for project_macro, ssc_macro, expected in zip(
        ['PROJECT_VENDOR_ID', 'PROJECT_PRODUCT_CODE', 'PROJECT_REVISION'],
        ['VENDOR_ID', 'PRODUCT_CODE', 'REVISION_NUMBER'],
        struct.unpack_from('<3I', data, 16)):
    for text, macro in [(identity_header, project_macro), (ssc_config, ssc_macro)]:
        match = re.search(r'^#define\s+' + macro + r'\s+(0x[0-9a-fA-F]+)', text, re.MULTILINE)
        check(match is not None and int(match[1], 16) == expected, macro + ' identity')
check(struct.unpack_from('<5H', data, 48) == (0x1000,128,0x1080,128,4), 'mailbox')
offset, categories = 128, {}
while struct.unpack_from('<H', data, offset)[0] != 0xffff:
    kind, words = struct.unpack_from('<HH', data, offset)
    check(offset+4+words*2 <= len(data), 'category bounds')
    categories[kind] = data[offset+4:offset+4+words*2]
    offset += 4+words*2
check(set(categories) == {10,30,40,41}, 'categories')
check(len(categories[41]) == 32, 'SM count')
check(struct.unpack_from('<HHBBBB',categories[41],16) == (0x1100,12,0x64,0,1,3), 'SM2')
check(struct.unpack_from('<HHBBBB',categories[41],24) == (0x1400,12,0x20,0,1,4), 'SM3')
image = ROOT / 'build/firmware-gcc/ethercat_joint.bin'
if image.exists():
    raw = image.read_bytes()
    stack, reset = struct.unpack_from('<II', raw)
    check(stack == 0x20020000 and len(raw) < 512*1024, 'Flash/SRAM layout')
    check(0x08000000 <= (reset & ~1) < 0x08000000+len(raw) and reset & 1, 'reset vector')
print('PASS: ESI/SII/SSC mapping, CRC, categories and available firmware image bounds')
