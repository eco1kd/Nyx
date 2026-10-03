from pathlib import Path
import hashlib
R=Path('/home/leftcode/Projects/LemmingRMT')
B=Path((R/'diagnostics/aim-debug/latest-features-backup.txt').read_text().strip())
for line in (B/'before-sha256.txt').read_text().splitlines():
 h,name=line.split(None,1);assert hashlib.sha256((R/name.strip()).read_bytes()).hexdigest()==h, 'Source changed since backup: '+name
assert 'v11-aim-hook-fix' in (R/'jni/src/game/offsets.hpp').read_text()
files={}
def read(p):
 if p not in files: files[p]=p.read_text()
 return files[p]
def save(p,s): files[p]=s
def replace(path,old,new):
 p=R/path;s=read(p);assert s.count(old)==1,(path,old);save(p,s.replace(old,new,1))
replace('jni/src/features/aim/core.hpp','onAttack=true,lockTarget=false;','onAttack=true,lockTarget=false,showFov=false,rcs=false,visibleCheck=false;')
replace('jni/src/features/aim/core.hpp','smoothing=.12f,pointScale=.5f;','smoothing=.12f,pointScale=.5f,rcsStrength=100;')
replace('jni/src/features/aim/core.hpp','inline std::atomic<int> g_normalState','inline std::atomic<bool> g_rcsReady{false},g_visibilityReady{false};\ninline std::atomic<int> g_normalState')
replace('jni/src/features/aim/core.hpp','return s;}','s.rcsStrength=finiteClamp(s.rcsStrength,100,0,100);return s;}')
replace('jni/src/features/aim/core.hpp','std::uintptr_t locked=0){int body=', 'std::uintptr_t locked=0,bool(*visible)(void*,std::uintptr_t,Vec3,Vec3)=nullptr,void* context=nullptr){int body=')
replace('jni/src/features/aim/core.hpp','))best={true,player,p[i],angle,distance,score,v.bone};', ')){if(s.visibleCheck&&(!visible||!visible(context,player,origin,p[i])))continue;best={true,player,p[i],angle,distance,score,v.bone};}')
replace('jni/src/features/aim/core.hpp','case 7:return "Profile / hook validation failed";','case 7:return "Profile / hook validation failed";case 8:return "Visibility profile is not ready";case 9:return "RCS only (no eligible target)";')
replace('jni/src/features/aim/offsets.hpp','inline constexpr std::uint8_t kAttackState=', '''// Verified native/dump provenance in docs/game/ANGLE_ASSISTS_1.0.0_ARM64.md.
inline constexpr std::uintptr_t kNativeRaycast=0x538BD0C;
inline constexpr std::uintptr_t kDefaultPhysicsScene=0x904D9C4;
inline constexpr std::uintptr_t kObjectEntityId=0x67F0658;
inline constexpr std::uintptr_t kRecoilScale=0x972976C;
inline constexpr std::uintptr_t kRecoilPointLoad=0x9736FD8;
inline constexpr std::uintptr_t kRecoilMultiplierLoad=0x97297B0;
inline constexpr std::size_t kRecoilActualPoint=0xA8,kRecoilMultiplier=0x70;
inline constexpr std::uint8_t kAttackState=''')
replace('jni/src/features/aim/debug.hpp','inline std::atomic<int> boneMin','inline Count rcsReads{0},rcsRejected{0},rcsApplied{0},visibilityRays{0},visibilityClear{0},visibilityTarget{0},visibilityBlocked{0},visibilityUnknown{0};\ninline std::atomic<float> recoilPitch{0},recoilYaw{0};\ninline std::atomic<int> boneMin')
replace('jni/src/features/aim/debug.hpp','case 14:return "non-finite-angles";','case 14:return "non-finite-angles";case 15:return "visibility-not-ready";case 16:return "recoil-only";')
p=R/'jni/src/features/aim/debug.hpp';s=read(p);at=s.rfind(');}')+2;assert at>1
s=s[:at]+'''\n log("assists showFov=%d visible[enabled=%d ready=%d rays=%llu clear=%llu target=%llu blocked=%llu unknown=%llu] rcs[enabled=%d ready=%d strength=%.1f reads=%llu rejected=%llu applied=%llu] recoil[pitch=%.3f yaw=%.3f]",c.normal.showFov,c.normal.visibleCheck,g_visibilityReady.load(),(unsigned long long)visibilityRays.load(),(unsigned long long)visibilityClear.load(),(unsigned long long)visibilityTarget.load(),(unsigned long long)visibilityBlocked.load(),(unsigned long long)visibilityUnknown.load(),c.normal.rcs,g_rcsReady.load(),c.normal.rcsStrength,(unsigned long long)rcsReads.load(),(unsigned long long)rcsRejected.load(),(unsigned long long)rcsApplied.load(),recoilPitch.load(),recoilYaw.load());'''+s[at:];save(p,s)
replace('jni/src/features/aim/runtime.hpp','#include "features/aim/hook_support.hpp"','#include "features/aim/hook_support.hpp"\n#include "features/aim/assists.hpp"')
replace('jni/src/features/aim/runtime.hpp','unsigned count=0;Clock::time_point captured','unsigned count=0;std::array<std::int32_t,33> colliderIds{};unsigned colliderCount=0;Clock::time_point captured')
replace('jni/src/features/aim/runtime.hpp','inline void capture(void* p){','// The next header deliberately shares this runtime namespace.\n#include "features/aim/assist_runtime.hpp"\ninline void capture(void* p,bool collectIds=false){')
replace('jni/src/features/aim/runtime.hpp','a.volumes[a.count++]=v;}','a.volumes[a.count++]=v;if(collectIds&&g_entityId&&a.colliderCount<a.colliderIds.size())a.colliderIds[a.colliderCount++]=g_entityId(c,nullptr);}\n if(collectIds&&g_entityId&&a.colliderCount<a.colliderIds.size()){void* bindings=object(part(p,"dwc"),0x38);void* capsule=object(bindings,0x28);if(named(bindings,"PlayerBindings")&&(named(capsule,"CapsuleCollider")||named(capsule,"CharacterController"))&&native(capsule))a.colliderIds[a.colliderCount++]=g_entityId(capsule,nullptr);}')
replace('jni/src/features/aim/runtime.hpp','debug::selectCalls.fetch_add(1);Selection best{};','debug::selectCalls.fetch_add(1);g_visibilityBudget=64;Selection best{};')
replace('jni/src/features/aim/runtime.hpp','origin,forward,v,last);','origin,forward,v,last,visibilityAccept,const_cast<Actor*>(&a));')
p=R/'jni/src/features/aim/runtime.hpp';s=read(p);a=s.index('inline void cameraTick(');b=s.index('inline void emitRays(',a)
s=s[:a]+'''inline void cameraTick(void* self,ExecuteTime time,const void* method){debug::cameraCalls.fetch_add(1);
 auto original=g_cameraOriginal.load(std::memory_order_acquire);if(original)original(self,time,method);if(!ed::isUnityMainCallback()){debug::normalReason.store(4);return;}if(!g_functionsValid){debug::normalReason.store(5);return;}auto c=configuration();auto s=c.normal;if(!gate(c,s,g_normalState)){g_recoilTracker.reset();debug::normalReason.store(!s.enabled?1:(c.pauseInMenu&&g_menuOpen.load()?2:3));return;}
 void* p=object(self,0x18);void* world=nullptr;ed::PlayerSnapshot local{};if(!localAlive(p,local,world)||object(self,0x20)!=world||!named(object(self,0x38),"eir")||!object(self,0x48)||!native(object(self,0x50),"Transform")||!native(object(self,0x58),"Transform")){g_recoilTracker.reset();debug::normalReason.store(7);g_normalState.store(3);return;}if(s.onAttack&&!attacking(local.weaponPart)){g_recoilTracker.reset();debug::normalReason.store(8);g_normalState.store(4);return;}
 Vec3 origin{},forward{};if(!transformPose(object(local.weaponPart,offsets::field::nrk::kHitTransform),origin,forward)){g_recoilTracker.reset();debug::normalReason.store(9);g_normalState.store(3);return;}
 auto target=select(s,world,local.team,origin,forward,g_lastNormal);auto current=anglesFor(forward);auto desired=target.valid?anglesFor(sub(target.point,origin)):current;bool recoilOnly=false,recoilUsed=false;Angles recoil{};
 if(s.rcs&&s.rcsStrength>0&&attacking(local.weaponPart)&&readRecoil(local.weaponPart,recoil)){
  auto weapon=reinterpret_cast<std::uintptr_t>(local.activeWeapon);
  if(target.valid){desired=assists::compensate(desired,recoil,s.rcsStrength);g_recoilTracker.synchronize(weapon,{recoil.pitch*s.rcsStrength*.01f,recoil.yaw*s.rcsStrength*.01f});recoilUsed=std::fabs(recoil.pitch)+std::fabs(recoil.yaw)>.0001f;}
  else{auto delta=g_recoilTracker.delta(weapon,{recoil.pitch*s.rcsStrength*.01f,recoil.yaw*s.rcsStrength*.01f},true);recoilOnly=std::fabs(delta.pitch)+std::fabs(delta.yaw)>.0001f;recoilUsed=recoilOnly;desired=assists::compensate(current,delta,100);}
 }else g_recoilTracker.reset();
 if(!target.valid&&!recoilOnly){bool unavailable=s.visibleCheck&&!g_visibilityReady.load();debug::normalReason.store(unavailable?15:10);g_normalState.store(unavailable?8:5);return;}
 auto next=recoilOnly?desired:smooth(current,desired,s.smoothing,time.delta);next=g_clamp(self,next,nullptr);if(!std::isfinite(next.pitch)||!std::isfinite(next.yaw)){debug::normalReason.store(14);return;}g_apply(self,next,nullptr);g_refresh(self,nullptr);g_normalApplications.fetch_add(1);if(recoilUsed)debug::rcsApplied.fetch_add(1);debug::normalReason.store(recoilOnly?16:11);g_normalState.store(recoilOnly?9:6);
}
'''+s[b:];save(p,s)
replace('jni/src/features/aim/runtime.hpp','capture(p);','prepareAssists(c.normal.rcs,c.normal.visibleCheck);capture(p,c.normal.visibleCheck&&g_visibilityReady.load());')
p=R/'jni/src/ui/pages/aim.hpp';save(p,(R/'diagnostics/aim-debug/aim_ui_v12.hpp').read_text())
replace('jni/src/ui/premium.hpp','renderEspBoxes();renderWatermark();','renderEspBoxes();renderAngleFov();renderWatermark();')
replace('jni/src/core/entry.cpp','#include "features/aim/runtime.hpp"','#include "features/aim/runtime.hpp"\n#include "features/aim/assists.hpp"')
replace('tests/ui/ui_test.cpp','#include "features/aim/core.hpp"','#include "features/aim/core.hpp"\n#include "features/aim/assists.hpp"')
replace('tests/ui/ui_test.cpp','auto mask=g_aimUi.normal.bodies;click(parts->Pos.x+240,parts->Pos.y+22);assert(g_aimUi.normal.bodies==(mask^(1u<<1)));assert(g_aimUi.silent.bodies==5);click(parts->Pos.x+150,parts->Pos.y+238);assert(g_aimUi.normal.multipoints);', '''click(parts->Pos.x+150,parts->Pos.y+22);assert(g_aimUi.normal.showFov);click(parts->Pos.x+150,parts->Pos.y+184);assert(g_aimUi.normal.visibleCheck);click(parts->Pos.x+150,parts->Pos.y+238);assert(g_aimUi.normal.rcs);assert(!g_aimUi.silent.rcs&&!g_aimUi.silent.visibleCheck);g_subsection=2;g_aimEditMode=0;settle();parts=containing("##aim_right");auto mask=g_aimUi.normal.bodies;click(parts->Pos.x+240,parts->Pos.y+22);assert(g_aimUi.normal.bodies==(mask^(1u<<1)));assert(g_aimUi.silent.bodies==5);click(parts->Pos.x+150,parts->Pos.y+238);assert(g_aimUi.normal.multipoints);''')
replace('scripts/test.sh',' run_case hook_support tests/aim/hook_support_test.cpp -ldl -lpthread',' run_case hook_support tests/aim/hook_support_test.cpp -ldl -lpthread\n run_case assists tests/aim/assists_test.cpp -ldl -lpthread')
replace('tests/aim/debug_smoke.cpp','assert(lines.size()==6);','assert(lines.size()==7);')
for name in ['jni/src/game/offsets.hpp','jni/src/features/aim/debug.hpp','tests/aim/debug_smoke.cpp','scripts/debug/aim_debug.sh']:
 p=R/name;s=read(p);assert 'v11-aim-hook-fix' in s;save(p,s.replace('v11-aim-hook-fix','v12-angle-assists').replace('No v11 heartbeat','No v12 heartbeat'))
import sys,os
if '--check' in sys.argv:
 print('Preflight passed for all',len(files),'source edits; no source changed');sys.exit(0)
for p,s in files.items():
 temporary=p.with_suffix(p.suffix+'.v12-tmp');temporary.write_text(s);os.replace(temporary,p)
for name in ['scripts/test.sh','scripts/debug/aim_debug.sh']: (R/name).chmod(0o755)
print('Applied v12 Angles FOV, RCS and per-point visibility; Silent handler/installers untouched')
