from pathlib import Path
import hashlib,json,re,tarfile
R=Path('/home/leftcode/Projects/LemmingRMT');B=Path((R/'diagnostics/latest-project-organization-backup.txt').read_text().strip())
manifest=json.loads((B/'before-manifest.json').read_text());moves=json.loads((R/'docs/architecture/file-moves.json').read_text())
source_map=json.loads((R/'docs/architecture/code-preservation.json').read_text())['sources']
def normalized(s):return re.sub(r'^\s*#include "[^"\n]+"\s*$', '',s,flags=re.M).replace('assets/icons/lucide/LICENSE','assets/lucide/LICENSE')
code=[]
with tarfile.open(B/'sources-and-release.tar.gz') as t:
 for a,b in source_map.items():
  if not a.startswith('jni/'):continue
  before=t.extractfile(a).read().decode();after=(R/b).read_text()
  assert normalized(before)==normalized(after),b
  code.append(b)
old_modified={'.gitignore','build.sh','aim_debug.sh','jni/Android.mk','docs/AIM_DEBUG.md','docs/AIM_OFFSETS_1.0.0_ARM64.md','diagnostics/premium-ui-20260930/tests/ui_test.cpp'}
checked=[];changed=[];errors=[]
for old,meta in manifest.items():
 new=old
 for a,b in moves:
  if new==a or new.startswith(a+'/'):new=b+new[len(a):]
 p=R/new
 if not p.is_file():errors.append({'old':old,'new':new,'reason':'missing'});continue
 if old.startswith('diagnostics/organization/'):continue
 if old in old_modified or old in source_map:changed.append({'old':old,'new':new,'reason':'documented source/include/build/test update'});continue
 if old.startswith('out/'):
  with tarfile.open(B/'sources-and-release.tar.gz') as t:
   h=hashlib.sha256(t.extractfile(old).read()).hexdigest()
  assert h==meta['sha256'];changed.append({'old':old,'new':new,'reason':'rebuilt output; old bytes verified in backup'});continue
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 if h.hexdigest()!=meta['sha256']:errors.append({'old':old,'new':new,'reason':'hash mismatch'})
 else:checked.append(new)
report={'backup':str(B),'runtime_sources_unchanged_except_include_paths':code,'preserved_files_sha256_verified':len(checked),'documented_changes':changed,'errors':errors,'phone_operations_performed':False}
(R/'diagnostics/organization/preservation.json').write_text(json.dumps(report,indent=2)+'\n')
assert not errors,errors
print(f'PASS preservation: {len(code)} runtime source bodies unchanged, {len(checked)} original files SHA-256 verified, previous output retained in backup')
