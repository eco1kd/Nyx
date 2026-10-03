from pathlib import Path
import json
R=Path('/home/leftcode/Projects/LemmingRMT')
def write(name,text,mode=None):
 p=R/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
 if mode:p.chmod(mode)
moves=json.loads((R/'docs/architecture/file-moves.json').read_text())
for a,b in [('README.md','docs/archive/README_before_organization.md'),('diagnostics/archive/loose/organize_project.py','scripts/maintenance/legacy/organize_project_20261001.py')]:
 src,dst=R/a,R/b;assert src.is_file() and not dst.exists();dst.parent.mkdir(parents=True,exist_ok=True);src.rename(dst);moves.append([a,b])
write('docs/architecture/file-moves.json',json.dumps(moves,indent=2)+'\n')
B=Path((R/'diagnostics/latest-project-organization-backup.txt').read_text().strip());(B/'moves.json').write_text(json.dumps(moves,indent=2))
write('test.sh','#!/usr/bin/env bash\nset -euo pipefail\nROOT="$(cd "$(dirname "$0")" && pwd)"\nexec "$ROOT/scripts/test.sh" "$@"\n',0o755)
(R/'scripts/test.sh').chmod(0o755)
for name,desc in [('scripts/assets','Future generators for embedded resources; original resources belong in assets/.'),('scripts/packaging','Future packaging helpers. Do not put generated releases here; use out/.'),('scripts/docs','Future documentation maintenance helpers.'),('tests/rage','Future Rage tests; no implementation exists yet.'),('tests/misc','Future Misc tests; no implementation exists yet.'),('tests/config','Future config parsing/serialization tests; no persistence exists yet.'),('tests/settings','Future shared-settings tests.'),('jni/src/ui/pages/visuals','Reserved for a future split of ESP controls/editor; current UI still lives in ui/premium.hpp.')]:
 write(name+'/README.md','# Reserved\n\n'+desc+'\n')
p=R/'tests/ui/ui_test.cpp';s=p.read_text();assert s.count('#include <fstream>')==1;s=s.replace('#include <fstream>','#include <fstream>\n#include <filesystem>\n#include <cstdlib>',1);assert 'std::ofstream f(path,std::ios::binary);f<<' in s;s=s.replace('std::ofstream f(path,std::ios::binary);f<<','std::ofstream f(path,std::ios::binary);assert(f.is_open());f<<',1)
a='const char* shots[]={"diagnostics/aim-angles.ppm","diagnostics/aim-silent.ppm","diagnostics/aim-targeting.ppm","diagnostics/aim-multipoints.ppm"};writePpm(shots[page]);'
b='const char* shots[]={"aim-angles.ppm","aim-silent.ppm","aim-targeting.ppm","aim-multipoints.ppm"};const char* dir=std::getenv("LEMMING_TEST_SCREENSHOT_DIR");std::filesystem::path folder=dir&&*dir?dir:"diagnostics/tests/manual/screenshots";std::filesystem::create_directories(folder);writePpm((folder/shots[page]).c_str());'
assert s.count(a)==1;s=s.replace(a,b,1);p.write_text(s)
write('README.md','''# LemmingRMT

ARM64 / GLES3 project for Standoff 2 1.0.0, organized by responsibility.

## Current state

The current profile remains v10-aim-debug. Experimental Angles and Silent runtime/settings exist, but their live failure is not yet diagnosed or fixed. Diagnostic reports are automatic; both modes default OFF, menu-pausing defaults ON. FOV is an angular target filter, not a drawn circle. Visibility/penetration, hitchance, recoil compensation, auto-fire, config persistence and anti-cheat bypass are NOT implemented. Rage/Config/etc. folders are scaffolding, not working features. Skeleton ESP remains removed.

## Quick commands

```bash
./build.sh                    # ARM64 library -> out/arm64-v8a/
./test.sh                     # current host tests + structure check
./test.sh unit                # math/runtime guards + diagnostic formatting
./test.sh ui                  # menu/layout and offscreen GLES3 when available
python3 scripts/check_structure.py
./aim_debug.sh capture 45      # bounded, tag-only ADB capture
./adb_logs.sh dump             # buffered module logs
```

Set ANDROID_NDK_HOME for a different NDK, CXX for a different host compiler, ADB / ANDROID_SERIAL for a different ADB/device. `./build.sh clean` preserves out/ and backups and cleans only NDK intermediate files. No clean was run during organization.

## Where things live

- jni/src/core/: existing bootstrap/render/input integration.
- jni/src/game/: versioned game profile.
- jni/src/features/aim/: settings/math, runtime, Aim offsets, debug counters.
- jni/src/features/visuals/: existing ESP runtime.
- jni/src/features/rage/, misc/, config/, settings/: reserved future sections.
- jni/src/ui/: menu/editor, Aim page and embedded resources; future page folders are reserved.
- jni/third_party/imgui/: vendor code and license.
- assets/: original icons/fonts and licenses.
- scripts/: build, debug, validation and test tooling.
- tests/: reusable current host test sources and Android stub.
- docs/: architecture, debug guides, game evidence and historical notes.
- reference/dumps/1.0.0/: original dump, unchanged.
- build/: generated NDK files and host binaries; previous caches retained under legacy-ndk/.
- out/arm64-v8a/liblemmingrmt.so: final library, unchanged output path.
- diagnostics/aim-debug/: current Aim captures, unchanged path.
- diagnostics/tests/: fresh test logs/screenshots.
- diagnostics/archive/: preserved older sessions/reports/screenshots.
- backups/: source/release snapshots; no previous backup was deleted.

Detailed map and extension conventions: [PROJECT_STRUCTURE.md](docs/architecture/PROJECT_STRUCTURE.md).
Debug guide: [AIM_DEBUG.md](docs/debug/AIM_DEBUG.md).
Offset evidence: [AIM_OFFSETS_1.0.0_ARM64.md](docs/game/AIM_OFFSETS_1.0.0_ARM64.md).
Renames: [file-moves.json](docs/architecture/file-moves.json).

The original README is preserved in docs/archive/README_before_organization.md. Previous root scripts are forwarding wrappers; old dump and document names remain compatibility symlinks. Code was moved, not redesigned internally: entry.cpp, ESP and the shared premium menu still contain the existing implementations.

Use modifies game behavior, violates the game's ToS and may result in a ban.
''')
write('tests/README.md','# Host tests\n\nRun ./test.sh [all|unit|ui] from the project root. test binaries belong under build/tests/, run logs/screenshots under diagnostics/tests/. Current cases cover Aim math/memory guards, debug logging and menu interaction/layout. They do not prove live hooks or server hit registration. Historical/removed-feature tests remain as archive evidence, not active test inputs.\n')
write('assets/README.md','# Assets\n\nicons/lucide/: original SVGs with licenses and provenance.\nfonts/mozilla_text/: original TTF and OFL license.\nEmbedded C++ arrays live under jni/src/ui/icons/ and ui/fonts/. Keep licenses when regenerating or distributing assets. No resource bytes were changed during organization.\n')
source_map=json.loads((R/'docs/architecture/code-preservation.json').read_text())['sources']
for name in ['docs/debug/AIM_DEBUG.md','docs/game/AIM_OFFSETS_1.0.0_ARM64.md']:
 p=R/name;s=p.read_text()
 for a,b in source_map.items():s=s.replace(a,b)
 p.write_text(s)
p=R/'scripts/debug/aim_debug.sh';s=p.read_text();s=s.replace('OUT="$ROOT/diagnostics/aim-debug"','OUT="${LEMMING_DIAGNOSTICS_DIR:-$ROOT/diagnostics/aim-debug}"');p.write_text(s)
for name in ['jni/fonts']:
 p=R/name
 if p.exists() and not any(p.iterdir()):p.rmdir()
print('Finalized source map, reserved modules, documentation and reusable tests')
