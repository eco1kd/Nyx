from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[1]
search=[root/'jni/src',root/'jni/third_party/imgui',root/'jni/third_party/imgui/backends',root/'tests/stubs']
errors=[];checked=0
for base in [root/'jni/src',root/'tests']:
 for p in base.rglob('*'):
  if p.suffix not in ['.cpp','.h','.hpp']:continue
  for name in re.findall(r'^\s*#\s*include\s+"([^"\n]+)"',p.read_text(),re.M):
   checked+=1
   if not any((d/name).is_file() for d in [p.parent]+search):errors.append(f'{p.relative_to(root)}: missing include {name}')
mk=(root/'jni/Android.mk').read_text().replace('\\\n',' ')
m=re.search(r'^LOCAL_SRC_FILES\s*:=\s*(.+)$',mk,re.M)
if not m:errors.append('Android.mk: LOCAL_SRC_FILES missing')
else:
 for name in m[1].split():
  if not (root/'jni'/name).is_file():errors.append(f'Android.mk: missing source {name}')
for p in ['build.sh','adb_logs.sh','aim_debug.sh','test.sh','scripts/build.sh','scripts/test.sh','scripts/debug/adb_logs.sh','scripts/debug/aim_debug.sh','jni/src/core/entry.cpp','jni/src/game/offsets.hpp','tests/aim/aim_test.cpp','tests/aim/debug_smoke.cpp','tests/ui/ui_test.cpp','tests/stubs/android/log.h','assets/fonts/mozilla_text/MozillaText-OFL.txt','assets/icons/lucide/LICENSE','jni/third_party/imgui/LICENSE.txt','docs/architecture/PROJECT_STRUCTURE.md']:
 if not (root/p).is_file():errors.append(f'Missing {p}')
for name in ['rage','misc','config','settings']:
 for base in ['jni/src/features','jni/src/ui/pages']:
  if not (root/base/name/'README.md').is_file():errors.append(f'Missing reserved {base}/{name}')
for p in root.rglob('*'):
 if 'backups' in p.relative_to(root).parts:continue
 if p.is_symlink() and not p.exists():errors.append(f'Broken symlink {p.relative_to(root)}')
if errors:
 print('\n'.join(errors),file=sys.stderr);sys.exit(1)
print(f'PASS structure: {checked} local includes resolved, build sources/licenses/reserved modules and compatibility links present')
