from pathlib import Path
import hashlib
import json
import re
import struct
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = ET.parse(root / 'KongTan_8dianji.vcxproj').getroot()
ns = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
sources = [node.attrib['Include'] for tag in ['ClCompile', 'ClInclude', 'ResourceCompile', 'Image']
           for node in project.findall('.//m:' + tag, ns) if 'Include' in node.attrib]
missing = [name for name in sources if not (root / name).is_file()]
assert not missing, missing
dialog = (root / 'KongTan_8dianjiDlg.cpp').read_text(encoding='utf-8-sig')
resource = (root / 'KongTan8dianji.rc').read_text(encoding='utf-8-sig')
ddx = re.findall(r'DDX_\w+\(pDX,\s*(IDC_\w+)', dialog)
assert ddx and all(name in resource for name in ddx)
assert '\ufffd' not in resource
binary = root / 'x64/Release/KongTan_9Axis_Linkage.exe'
data = binary.read_bytes()
offset = struct.unpack_from('<I', data, 0x3c)[0]
assert data[:2] == b'MZ' and struct.unpack_from('<H', data, offset + 4)[0] == 0x8664
assert 'Confirmed=0' in (root / 'x64/Release/machine.ini').read_text()
result = {'project_sources': len(sources), 'bound_controls': len(ddx), 'missing_files': missing,
          'release_exe_bytes': len(data), 'release_sha256': hashlib.sha256(data).hexdigest(),
          'default_hardware_config': 'unconfirmed placeholder; no automatic enable'}
(root / 'tests/package-results.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
print(json.dumps(result, indent=2))
print('Project and release package checks passed.')
