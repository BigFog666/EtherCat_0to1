"""分析真实主站 CSV；统计 PC 时间，不能解释成线缆延迟或 DC 精度。"""
import argparse
import csv
import statistics
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('file', type=Path)
args = parser.parse_args()
with args.file.open(encoding='utf-8-sig', newline='') as f:
    reader = csv.DictReader(f)
    required = {'cycle','phase','interval_us','roundtrip_us','wkc','expected_wkc','valid'}
    if not required.issubset(reader.fieldnames or []):
        raise SystemExit('不是 joint_master 的硬件 CSV 格式')
    rows = list(reader)
if not rows:
    raise SystemExit('没有样本')
valid = [r for r in rows if int(r['valid']) == 1 and int(r['wkc']) == int(r['expected_wkc'])]
print('samples=',len(rows),'valid=',len(valid),'invalid=',len(rows)-len(valid))
for name in ['interval_us','roundtrip_us']:
    values = sorted(int(r[name]) for r in valid if name != 'interval_us' or int(r['cycle']) > 0)
    if values:
        p99 = values[max(0, (99*len(values)+99)//100-1)]
        print(name,'min=',min(values),'mean=',round(statistics.mean(values),2),
              'p99=',p99,'max=',max(values),'stdev=',round(statistics.pstdev(values),2))
print('phases=',','.join(sorted({r['phase'] for r in rows},key=int)))
print('仅为 PC 调用时间统计；具体阶段是否完成以主站退出结果和板端反馈为准。')
