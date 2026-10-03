from pathlib import Path
import datetime,hashlib,json,re,shutil,tarfile
R=Path('/home/leftcode/Projects/LemmingRMT')
assert (R/'jni/main.cpp').is_file() and not (R/'jni/src').exists()
B=R/'backups'/(datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'-before-project-organization')
B.mkdir(parents=True)
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
manifest={}
for p in R.rglob('*'):
 if p.is_file() and 'backups' not in p.relative_to(R).parts and not p.is_symlink():
  manifest[str(p.relative_to(R))]={'size':p.stat().st_size,'sha256':digest(p)}
(B/'before-manifest.json').write_text(json.dumps(manifest,indent=2))
with tarfile.open(B/'sources-and-release.tar.gz','w:gz') as t:
 for name in ['jni','assets','docs','README.md','.gitignore','build.sh','adb_logs.sh','aim_debug.sh','out','diagnostics/aim_test.cpp','diagnostics/aim-debug/aim_debug_smoke.cpp','diagnostics/premium-ui-20260930/tests/ui_test.cpp','diagnostics/premium-ui-20260930/tests/android/log.h']:
  t.add(R/name,arcname=name)
moves=[]
def move(a,b):
 a,b=R/a,R/b
 assert a.exists() and not b.exists(),(a,b)
 b.parent.mkdir(parents=True,exist_ok=True);a.rename(b);moves.append([str(a.relative_to(R)),str(b.relative_to(R))])
def write(name,text,mode=None):
 p=R/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
 if mode:p.chmod(mode)
source_moves={
 'jni/main.cpp':'jni/src/core/entry.cpp',
 'jni/game_offsets.hpp':'jni/src/game/offsets.hpp',
 'jni/esp_runtime.hpp':'jni/src/features/visuals/esp_runtime.hpp',
 'jni/aim_core.hpp':'jni/src/features/aim/core.hpp',
 'jni/aim_offsets.hpp':'jni/src/features/aim/offsets.hpp',
 'jni/aim_runtime.hpp':'jni/src/features/aim/runtime.hpp',
 'jni/aim_debug.hpp':'jni/src/features/aim/debug.hpp',
 'jni/aim_ui.hpp':'jni/src/ui/pages/aim.hpp',
 'jni/ui_premium.hpp':'jni/src/ui/premium.hpp',
 'jni/lucide_vectors.hpp':'jni/src/ui/icons/lucide_vectors.hpp',
 'jni/fonts/mozilla_text.hpp':'jni/src/ui/fonts/mozilla_text.hpp',
 'diagnostics/aim_test.cpp':'tests/aim/aim_test.cpp',
 'diagnostics/aim-debug/aim_debug_smoke.cpp':'tests/aim/debug_smoke.cpp',
 'diagnostics/premium-ui-20260930/tests/ui_test.cpp':'tests/ui/ui_test.cpp',
 'diagnostics/premium-ui-20260930/tests/android/log.h':'tests/stubs/android/log.h'}
include_map={'game_offsets.hpp':'game/offsets.hpp','esp_runtime.hpp':'features/visuals/esp_runtime.hpp','aim_core.hpp':'features/aim/core.hpp','aim_offsets.hpp':'features/aim/offsets.hpp','aim_runtime.hpp':'features/aim/runtime.hpp','aim_debug.hpp':'features/aim/debug.hpp','aim_ui.hpp':'ui/pages/aim.hpp','ui_premium.hpp':'ui/premium.hpp','fonts/mozilla_text.hpp':'ui/fonts/mozilla_text.hpp','lucide_vectors.hpp':'ui/icons/lucide_vectors.hpp','imgui/imgui.h':'imgui.h','imgui/imgui_internal.h':'imgui_internal.h','imgui/backends/imgui_impl_opengl3.h':'backends/imgui_impl_opengl3.h'}
normal=lambda s:re.sub(r'^\s*#include "[^"\n]+"\s*$', '',s,flags=re.M).replace('assets/icons/lucide/LICENSE','assets/lucide/LICENSE')
normalized={a:normal((R/a).read_text()) for a in source_moves}
for a,b in source_moves.items():move(a,b)
move('jni/imgui','jni/third_party/imgui')
move('jni/assets','assets/fonts/mozilla_text')
move('assets/lucide','assets/icons/lucide')
for a,b in source_moves.items():
 p=R/b;s=p.read_text();s=re.sub(r'(#include ")([^"\n]+)(")',lambda m:m[1]+include_map.get(m[2],m[2])+m[3],s)
 if b.endswith('lucide_vectors.hpp'):s=s.replace('assets/lucide/LICENSE','assets/icons/lucide/LICENSE')
 assert normal(s)==normalized[a],('Non-include code changed',b)
 p.write_text(s)
for p in (R/'jni').glob('*.bak'):move(str(p.relative_to(R)),'backups/legacy-single-files/'+p.name)
for name in ['build.sh','adb_logs.sh','aim_debug.sh']:
 target='scripts/build.sh' if name=='build.sh' else 'scripts/debug/'+name
 move(name,target)
 write(name,'#!/usr/bin/env bash\nset -euo pipefail\nROOT="$(cd "$(dirname "$0")" && pwd)"\nexec "$ROOT/'+target+'" "$@"\n',0o755)
p=R/'scripts/debug/aim_debug.sh';s=p.read_text();s=s.replace('ROOT="$(cd "$(dirname "$0")" && pwd)"','ROOT="$(cd "$(dirname "$0")/../.." && pwd)"');p.write_text(s);p.chmod(0o755)
(R/'scripts/debug/adb_logs.sh').chmod(0o755)
move('dump 1.0.0','reference/dumps/1.0.0');(R/'dump 1.0.0').symlink_to('reference/dumps/1.0.0',target_is_directory=True)
for name in ['obj','libs']:move(name,'build/legacy-ndk/'+name)
for name in ['aim_test','ui_test','aim_debug_smoke']:
 p=R/'diagnostics/aim-debug'/name
 if p.exists():move(str(p.relative_to(R)),'build/tests/legacy-v10/'+name)
move('diagnostics/aim_test','build/tests/legacy-v9/aim_test')
move('diagnostics/aim-debug/add_aim_debug.py','scripts/maintenance/legacy/add_aim_debug_20261001.py')
for p in list((R/'diagnostics').iterdir()):
 if p.is_dir() and p.name not in ('aim-debug','organization'):move(str(p.relative_to(R)),'diagnostics/archive/sessions/'+p.name)
 elif p.is_file():
  dest='diagnostics/archive/screenshots/'+p.name if p.suffix.lower() in ['.ppm','.png','.jpg','.b64'] else 'diagnostics/archive/loose/'+p.name
  move(str(p.relative_to(R)),dest)
write('diagnostics/latest-project-organization-backup.txt',str(B)+'\n')
write('jni/Android.mk','''LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := lemmingrmt
LOCAL_SRC_FILES := src/core/entry.cpp \\
    third_party/imgui/imgui.cpp \\
    third_party/imgui/imgui_draw.cpp \\
    third_party/imgui/imgui_tables.cpp \\
    third_party/imgui/imgui_widgets.cpp \\
    third_party/imgui/backends/imgui_impl_opengl3.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src $(LOCAL_PATH)/third_party/imgui $(LOCAL_PATH)/third_party/imgui/backends
LOCAL_CPPFLAGS := -DIMGUI_IMPL_OPENGL_ES3 -std=c++17 -O2 -ffunction-sections -fdata-sections -fvisibility=hidden -Wall -Wextra
LOCAL_LDFLAGS := -Wl,--gc-sections
LOCAL_LDLIBS := -llog -lEGL -lGLESv3 -landroid -ldl
include $(BUILD_SHARED_LIBRARY)
''')
write('scripts/build.sh','''#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-$ROOT/../android-ndk-r30-linux/android-ndk-r30}"
NDK_BUILD="$NDK/build/ndk-build"
[[ -x "$NDK_BUILD" ]] || { echo "NDK build tool not found: $NDK_BUILD" >&2; exit 1; }
cd "$ROOT"
COMMON=(NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk "NDK_OUT=$ROOT/build/ndk/obj" "NDK_LIBS_OUT=$ROOT/build/ndk/libs")
case "${1:-}" in
 clean) "$NDK_BUILD" "${COMMON[@]}" clean; echo 'Intermediate files cleaned; out/ and historical backups are preserved.'; exit 0 ;;
 '' ) ;;
 *) echo "Usage: $0 [clean]" >&2; exit 2 ;;
esac
"$NDK_BUILD" "${COMMON[@]}" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
mkdir -p "$ROOT/out/arm64-v8a"
cp -f "$ROOT/build/ndk/libs/arm64-v8a/liblemmingrmt.so" "$ROOT/out/arm64-v8a/"
echo "Built: $ROOT/out/arm64-v8a/liblemmingrmt.so"
''',0o755)
write('.gitignore','''/build/
/out/
/obj/
/libs/
/backups/
/diagnostics/
/reference/dumps/
/dump 1.0.0
*.log
*.pid
*.exit
*.bak
__pycache__/
''')
for name in ['rage','misc','config','settings']:
 write('jni/src/features/'+name+'/README.md',f'# {name.title()} — reserved\n\nDirectory prepared for future implementation. No runtime code or feature is enabled by this placeholder.\nKeep feature configuration and runtime here; its UI belongs in ui/pages/{name}/.\n')
 write('jni/src/ui/pages/'+name+'/README.md',f'# {name.title()} UI — reserved\n\nFuture UI only. Runtime logic belongs in features/{name}/. This folder is not an implemented feature.\n')
for name in ['core','game','features/aim','features/visuals']:
 write('jni/src/'+name+'/README.md','# '+name+'\n\n'+{'core':'entry.cpp contains the existing bootstrap, overlay rendering and input integration. It was moved, not split or functionally rewritten.','game':'offsets.hpp is the current 1.0.0 ARM64 game profile. Keep versioned evidence in docs/game/.','features/aim':'core.hpp: settings and math; runtime.hpp: game integration; offsets.hpp: Aim RVAs; debug.hpp: counters/reports. Aim UI is in ui/pages/aim.hpp. Live validation is still pending.','features/visuals':'esp_runtime.hpp is the existing ESP runtime. Presentation and appearance stay in ui/premium.hpp. No skeleton runtime was restored.'}[name]+'\n')
for a,b in [('docs/AIM_DEBUG.md','docs/debug/AIM_DEBUG.md'),('docs/ADB_DEBUG.md','docs/debug/ADB_DEBUG.md'),('docs/AIM_OFFSETS_1.0.0_ARM64.md','docs/game/AIM_OFFSETS_1.0.0_ARM64.md'),('docs/Standoff2_1.0.0_ARM64_first_offsets_from_dump.txt','docs/archive/Standoff2_1.0.0_ARM64_first_offsets_from_dump.txt')]:
 move(a,b);(R/a).symlink_to(Path(b).relative_to('docs'))
write('docs/architecture/file-moves.json',json.dumps(moves,indent=2)+'\n')
write('docs/architecture/code-preservation.json',json.dumps({'all_moved_source_bodies_unchanged_except_include_paths':True,'sources':source_moves,'backup':str(B)},indent=2)+'\n')
write('scripts/maintenance/legacy/README.md','# Historical one-off migrations\n\nThese scripts describe already-applied changes and are not safe repeatable project commands. Do not run them automatically. Older session-specific patch scripts are retained under diagnostics/archive/sessions/.\n')
write('reference/README.md','# Local references\n\ndumps/1.0.0 contains the original dump files unchanged. The old root name "dump 1.0.0" is a compatibility symlink. Native ELF/APK references outside this repository were not moved. Do not commit dumps.\n')
write('diagnostics/README.md','# Diagnostics\n\naim-debug/: active bounded Aim captures and previous v10 results.\narchive/sessions/: preserved dated diagnostic sessions (historical test sources/binaries may remain as evidence).\narchive/loose/: old reports, build logs and pointer files.\narchive/screenshots/: previous PNG/JPG/PPM/base64 previews.\nNew reusable test sources are under tests/, test binaries under build/tests/, fresh test logs/screenshots under diagnostics/tests/. Nothing was deleted.\n')
(B/'moves.json').write_text(json.dumps(moves,indent=2))
print('BACKUP',B)
print('RENAMES',len(moves))
print('PASS moved source bodies unchanged apart from include paths')
