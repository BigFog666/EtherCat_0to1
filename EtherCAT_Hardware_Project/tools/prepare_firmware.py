"""从用户原 ZIP 准备本地 SSC 依赖，不执行归档中的程序。"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ARCHIVE = ROOT.parent.parent / 'STM32F407+LAN9252 EtherCAT从站/06_EtherCAT开发板源代码/EtherCAT_IO例程/IO例程_SPI_OLED+LED+KEY.zip'


def text_decode(raw):
    try:
        return raw.decode('utf-8-sig').replace('\r\r\n', '\n').replace('\r\n', '\n')
    except UnicodeDecodeError:
        return raw.decode('gb18030').replace('\r\r\n', '\n').replace('\r\n', '\n')


def replace_body(text, function, body):
    match = re.search(r'\b' + re.escape(function) + r'\s*\([^;]*?\)\s*\{', text)
    if not match:
        raise ValueError('未找到指定函数：' + function)
    begin = match.end() - 1
    depth, end = 1, begin + 1
    while depth:
        if text[end] == '{':
            depth += 1
        elif text[end] == '}':
            depth -= 1
        end += 1
    return text[:begin] + '{\n' + body + '\n}' + text[end:]


def save(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    # 生成文件可重复生成；先读取当前内容，避免无意义重写。
    if path.exists() and path.read_bytes() == content:
        return
    path.write_bytes(content)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--archive', type=Path, default=DEFAULT_ARCHIVE)
    args = parser.parse_args()
    archive = args.archive.resolve(strict=True)
    sdk = (ROOT / 'vendor/board_sdk').resolve()
    generated = ROOT / 'firmware/generated'
    esi = ET.parse(ROOT / 'config/EtherCAT_Joint_F407.xml')
    dtype = esi.find('.//Device/Type')
    identity = {'vendor': esi.findtext('Vendor/Id').replace('#x', '0x'),
                'product': dtype.get('ProductCode').replace('#x', '0x'),
                'revision': dtype.get('RevisionNo').replace('#x', '0x')}
    with zipfile.ZipFile(archive) as z:
        project_names = [s for s in z.namelist() if s.endswith('/MDK-ARM/Project.uvprojx')]
        if len(project_names) != 1:
            raise ValueError('归档不是已核对的 SPI IO 基线')
        project_name = project_names[0]
        prefix = project_name.split('/STM32F407 Ethercat/')[0] + '/'
        for info in z.infolist():
            if info.is_dir() or not info.filename.startswith(prefix):
                continue
            relative = info.filename[len(prefix):]
            try:
                relative = relative.encode('cp437').decode('gbk')
            except (UnicodeError, LookupError):
                pass
            if Path(relative).suffix.lower() not in {'.c', '.h', '.s', '.uvprojx', '.txt', '.pdf', '.xml', '.rst', '.md'}:
                continue
            target = (sdk / relative).resolve()
            if not target.is_relative_to(sdk):
                raise ValueError('归档路径越界：' + relative)
            raw = z.read(info)
            if target.exists() and target.read_bytes() != raw:
                raise ValueError('本地 SDK 文件已修改，请先保留修改：' + str(target))
            save(target, raw)
        board = sdk / 'STM32F407 Ethercat'
        # 将全部 SSC 头文件放在同一目录，避免双引号 include 命中原始配置。
        for source_header in (board / 'Ethercat/Inc').glob('*.h'):
            if source_header.name in {'cia402appl.h', 'ecat_def.h'}:
                continue  # 下方直接生成补丁版，避免每次还原再改造成全量重编。
            save(generated / source_header.name, text_decode(source_header.read_bytes()).encode('utf-8'))
        app = text_decode((board / 'Ethercat/src/cia402appl.c').read_bytes())
        header = text_decode((board / 'Ethercat/Inc/cia402appl.h').read_bytes())
        config = text_decode((board / 'Ethercat/Inc/ecat_def.h').read_bytes())
        for macro, value in {'EL9800_APPLICATION': '0', 'CiA402_DEVICE': '1', 'DC_SUPPORTED': '0',
                             'VENDOR_ID': identity['vendor'], 'PRODUCT_CODE': identity['product'],
                             'REVISION_NUMBER': identity['revision']}.items():
            config, count = re.subn(r'(?m)^(#define\s+' + macro + r'\s+)\S+',
                                    lambda m: m[1] + value, config)
            if count != 1:
                raise ValueError('配置宏定位失败：' + macro)
        header = header.replace('= {1,{0x1602,0x0}}', '= {1,{0x1600,0x0}}')
        header = header.replace('= {1,{0x1A02,0x0}}', '= {1,{0x1A00,0x0}}')
        if '= {1,{0x1600,0x0}}' not in header or '= {1,{0x1A00,0x0}}' not in header:
            raise ValueError('默认 PDO 分配定位失败')
        app = '#include "ssc_bridge.h"\n' + app
        app = replace_body(app, 'CiA402_StateMachine', '    /* 项目状态只在主循环的 Project_Poll 更新。 */')
        app = replace_body(app, 'CiA402_Application', '    (void)pCiA402Axis; /* ISR 不运行模型。 */')
        app = replace_body(app, 'CiA402_TransitionAction',
                           '    (void)Characteristic; (void)pCiA402Axis; return FALSE; /* 模型自行处理停止。 */')
        app = replace_body(app, 'APPL_OutputMapping', '    Project_OutputMapping(pData);')
        app = replace_body(app, 'APPL_InputMapping', '    Project_InputMapping(pData);')
        app = replace_body(app, 'APPL_GenerateMapping', '''    if (sRxPDOassign.u16SubIndex0 != 1 || sTxPDOassign.u16SubIndex0 != 1 ||
        sRxPDOassign.aEntries[0] != 0x1600 || sTxPDOassign.aEntries[0] != 0x1A00)
        return ALSTATUSCODE_NOVALIDOUTPUTS;
    if (!LocalAxes[0].bAxisIsActive) {
        TOBJECT OBJMEM *entry = LocalAxes[0].ObjDic;
        while (entry->Index != 0xFFFF) {
            UINT16 result = COE_AddObjectToDic(entry);
            if (result != 0) return result;
            entry++;
        }
        LocalAxes[0].bAxisIsActive = TRUE;
        LocalAxes[0].Objects.objSupportedDriveModes = 0x180;
    }
    *pInputSize = 12;
    *pOutputSize = 12;
    return ALSTATUSCODE_NOERROR;''')
        app = app.replace('    MainInit();', '    Project_Init();\n    MainInit();')
        app = app.replace('        MainLoop();', '        MainLoop();\n        Project_Poll();')
        if app.count('Project_Init();') != 1 or app.count('Project_Poll();') != 1:
            raise ValueError('应用入口定位失败')
        save(generated / 'cia402appl.c', app.encode('utf-8'))
        save(generated / 'cia402appl.h', header.encode('utf-8'))
        save(generated / 'ecat_def.h', config.encode('utf-8'))
        # MEM_ADDR 表示 SSC 的 16 位内存访问单位，不能全局改成指针类型。
        # 只修复对象读函数中的地址运算，Cortex-M 地址宽度为 32 位。
        objdef = text_decode((board / 'Ethercat/src/objdef.c').read_bytes())
        objdef = objdef.replace('((UINT16)pVarPtr)', '((UINT32)pVarPtr)')
        objdef = objdef.replace('(((MEM_ADDR)pVarPtr)& ~(MEM_ADDR)0x1)',
                                '(((UINT32)pVarPtr)& ~(UINT32)0x1)')
        save(generated / 'objdef.c', objdef.encode('utf-8'))
        hw = text_decode((board / 'Ethercat/port/el9800hw.c').read_bytes())
        hw = hw.replace('UINT16 intMask;', 'UINT32 intMask;')
        hw = 'extern void EXTI0_Configuration(void);\nvolatile unsigned int project_byte_test, project_hw_cfg;\n' + hw
        hw = hw.replace('SPI1_GPIO_Init();', 'SPI1_GPIO_Init();\n    project_byte_test = SPIReadDWord(0x64);\n    project_hw_cfg = SPIReadDWord(0x74);')
        save(generated / 'el9800hw.c', hw.encode('utf-8'))
        for source_name in ['coeappl.c', 'ecatappl.c', 'ecatslv.c']:
            source = text_decode((board / 'Ethercat/src' / source_name).read_bytes())
            source = source.replace('#include "el9800appl.h"', '#include "cia402appl.h"')
            save(generated / source_name, source.encode('utf-8'))

        project = ET.fromstring(z.read(project_name))
        old_base = board / 'MDK-ARM'
        for file in project.findall('.//File'):
            name = file.findtext('FileName')
            file_path = file.find('FilePath')
            old = (old_base / file_path.text.replace('\\', '/')).resolve()
            if name == 'el9800appl.c':
                file.find('FileName').text = 'cia402appl.c'
                file_path.text = 'cia402appl.c'
            elif name in {'objdef.c', 'el9800hw.c', 'coeappl.c', 'ecatappl.c', 'ecatslv.c'}:
                file_path.text = name
            else:
                file_path.text = os.path.relpath(old, generated).replace('/', '\\')
                if not old.is_file():
                    raise FileNotFoundError(old)
        include = project.find('.//Cads/VariousControls/IncludePath')
        includes = [str(generated), str(ROOT / 'firmware'), str(ROOT / 'common')]
        includes += [str((old_base / part.replace('\\', '/')).resolve()) for part in include.text.split(';') if part]
        include.text = ';'.join(os.path.relpath(Path(p), generated) for p in includes)
        group = ET.SubElement(project.find('.//Groups'), 'Group')
        ET.SubElement(group, 'GroupName').text = 'Project Application'
        files = ET.SubElement(group, 'Files')
        for source in [ROOT / 'firmware/ssc_bridge.c', ROOT / 'common/joint.c', ROOT / 'common/pdo.c']:
            item = ET.SubElement(files, 'File')
            ET.SubElement(item, 'FileName').text = source.name
            ET.SubElement(item, 'FileType').text = '1'
            ET.SubElement(item, 'FilePath').text = os.path.relpath(source, generated)
        project.find('.//TargetName').text = 'EtherCAT_Joint_F407'
        project.find('.//Device').text = 'STM32F407ZE'
        cpu = project.find('.//Cpu')
        if cpu is not None:
            cpu.text = cpu.text.replace('IROM(0x8000000,0x100000)', 'IROM(0x8000000,0x80000)')
        project.find('.//OutputName').text = 'ethercat_joint'
        project.find('.//OutputDirectory').text = '../../build/firmware/'
        project.find('.//ListingPath').text = '../../build/firmware/'
        project.find('.//CreateHexFile').text = '1'
        various = project.find('.//Cads/VariousControls')
        misc = various.find('MiscControls')
        misc.text = (misc.text or '') + ' --c99'
        generated.mkdir(parents=True, exist_ok=True)
        (ROOT / 'build/firmware').mkdir(parents=True, exist_ok=True)
        save(generated / 'Project.uvprojx', ET.tostring(project, encoding='utf-8', xml_declaration=True))
        sources = [(generated / item.findtext('FilePath').replace('\\', '/')).resolve()
                   for item in project.findall('.//File')
                   if item.findtext('FileName').lower().endswith('.c')]
        cmake = 'set(BOARD_SOURCES\n' + ''.join('  "' + p.as_posix() + '"\n' for p in sources) + ')\n'
        cmake += 'set(BOARD_INCLUDES\n' + ''.join('  "' + Path(p).as_posix() + '"\n' for p in includes) + ')\n'
        save(generated / 'GCCSources.cmake', cmake.encode('utf-8'))
    manifest = {'source_archive': str(archive), 'sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
                'baseline': 'SPI IO standard peripheral library + supplied SSC 5.11',
                'project': 'CMakeLists.txt',
                'generated_sources': 'firmware/generated/GCCSources.cmake',
                'soem_commit': '88e8ed46efba7dfa7b94d08a512db25a33e3f8d5',
                'identity': identity,
                'hardware_validation_record': 'evidence/2026-10-05_首次硬件联调.md',
                'sii_strategy': 'retain factory EEPROM; align firmware and master identity'}
    save(ROOT / 'config/source_manifest.json', (json.dumps(manifest, ensure_ascii=False, indent=2)+'\n').encode('utf-8'))
    print('Prepared CMake/GCC firmware sources; no flash/EEPROM operation performed.')


if __name__ == '__main__':
    main()
