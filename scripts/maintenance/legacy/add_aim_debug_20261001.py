from pathlib import Path
import shutil,datetime
r=Path('/home/leftcode/Projects/LemmingRMT')
b=r/'backups'/(datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'-before-aim-debug')
b.mkdir(parents=True)
for name in ['jni/main.cpp','jni/aim_runtime.hpp','jni/aim_core.hpp','jni/aim_ui.hpp','jni/game_offsets.hpp','README.md','adb_logs.sh','docs/ADB_DEBUG.md','out/arm64-v8a/liblemmingrmt.so']:
 p=b/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(r/name,p)
(r/'diagnostics/latest-aim-debug-backup.txt').write_text(str(b)+'\n')
def rep(s,old,new):
 assert s.count(old)==1,(old[:80],s.count(old))
 return s.replace(old,new,1)
p=r/'jni/aim_runtime.hpp';s=p.read_text();s=rep(s,'#include "aim_offsets.hpp"','#include "aim_offsets.hpp"\n#include "aim_debug.hpp"')
s=rep(s,'inline void capture(void* p){','inline void capture(void* p){debug::captureCalls.fetch_add(1);')
s=rep(s,'ed::PlayerSnapshot s{};if(!ed::readPlayerSnapshot(p,s)||s.localOwned||!s.statsValid||!s.hp||!ed::validTeam(s.team))return;','ed::PlayerSnapshot s{};if(!ed::readPlayerSnapshot(p,s)||!s.statsValid||!s.hp||!ed::validTeam(s.team)){debug::captureSnapshot.fetch_add(1);return;}if(s.localOwned){debug::captureLocal.fetch_add(1);return;}')
s=rep(s,'a.captured=Clock::now();if(!a.world)return;','a.captured=Clock::now();if(!a.world){debug::captureWorld.fetch_add(1);return;}')
s=rep(s,'if(!field(arr,0x18,count)||!count||count>64)return;','if(!field(arr,0x18,count)||!count||count>64){debug::captureArray.fetch_add(1);return;}')
s=rep(s,'esp::Vec3 root{};if(!ed::readPlayerPoseSnapshot(p,root))return;','esp::Vec3 root{};if(!ed::readPlayerPoseSnapshot(p,root)){debug::capturePose.fetch_add(1);return;}')
s=rep(s,'if(!named(hb,"HitBoxCollider"))continue;int bone=-1;if(!field(hb,0x20,bone)||bodyForBone(bone)<0)continue;','debug::hitboxes.fetch_add(1);if(!named(hb,"HitBoxCollider")){debug::badClass.fetch_add(1);continue;}int bone=-1;if(!field(hb,0x20,bone)){debug::unknownBone.fetch_add(1);continue;}debug::boundsBone(bone);if(bodyForBone(bone)<0){debug::unknownBone.fetch_add(1);continue;}')
s=rep(s,'if(!named(c,"BoxCollider")&&!named(c,"CapsuleCollider")&&!named(c,"SphereCollider"))continue;','if(!named(c,"BoxCollider")&&!named(c,"CapsuleCollider")&&!named(c,"SphereCollider")){debug::badCollider.fetch_add(1);continue;}')
s=rep(s,'auto ptr=native(c);std::uintptr_t vt=0,fn=0;if(!ptr||!ed::readValue(reinterpret_cast<void*>(ptr),vt)||!ed::readValue(reinterpret_cast<void*>(vt+0xf0),fn)||!unityCode(fn))continue;','auto ptr=native(c);if(!ptr){debug::badNative.fetch_add(1);continue;}std::uintptr_t vt=0,fn=0;if(!ed::readValue(reinterpret_cast<void*>(ptr),vt)||!ed::readValue(reinterpret_cast<void*>(vt+0xf0),fn)||!unityCode(fn)){debug::badVirtual.fetch_add(1);continue;}')
s=rep(s,'if(!validVolume(v)||length(sub(v.center,{root.x,root.y,root.z}))>4)continue;','if(!validVolume(v)){debug::badBounds.fetch_add(1);continue;}if(length(sub(v.center,{root.x,root.y,root.z}))>4){debug::farBounds.fetch_add(1);continue;}')
s=rep(s,'if(dst)*dst=a;','if(dst){*dst=a;debug::storedActors.fetch_add(1);debug::storedVolumes.fetch_add(a.count);}\n int actors=0,volumes=0;for(const auto& slot:g_actors)if(slot.count&&a.captured-slot.captured<std::chrono::milliseconds(100)){++actors;volumes+=slot.count;}debug::freshActors.store(actors);debug::freshVolumes.store(volumes);')
start=s.index('inline Selection select(');end=s.index('inline bool transformPose',start)
s=s[:start]+'''inline Selection select(const Settings& s,void* world,int team,Vec3 origin,Vec3 forward,std::uintptr_t& last){
 debug::selectCalls.fetch_add(1);Selection best{};auto now=Clock::now();float nearest=180,nearestRange=0;
 for(const auto& a:g_actors){if(!a.player)continue;debug::selectActors.fetch_add(1);if(a.world!=world||a.hp<=0||a.team==team||now-a.captured>std::chrono::milliseconds(100))continue;debug::selectFresh.fetch_add(1);
 std::uint16_t hp=0;auto stats=object(a.player,offsets::field::ake::kStats);if(ed::objectClass(a.player)!=ed::g_runtime.playerClass||object(a.player,offsets::field::mfc::kWorld)!=world||!field(stats,offsets::field::ylw::kHp,hp)||!hp)continue;debug::selectLive.fetch_add(1);
 for(unsigned i=0;i<a.count;++i){debug::selectVolumes.fetch_add(1);auto& v=a.volumes[i];int body=bodyForBone(v.bone);if(body<0||!(s.bodies&(1u<<body)))debug::bodyReject.fetch_add(1);else{std::array<Vec3,7> ps{};auto n=samples(v,s,ps);auto f=normalized(forward);for(unsigned j=0;j<n;++j){debug::pointCandidates.fetch_add(1);auto d=sub(ps[j],origin);float range=length(d);if(!std::isfinite(range)||range<.2f||range>s.maxDistance){debug::rangeReject.fetch_add(1);continue;}float angle=std::acos(std::clamp(dot(normalized(d),f),-1.f,1.f))*57.295779513f;if(angle<nearest){nearest=angle;nearestRange=range;}if(!std::isfinite(angle)||angle>s.fov)debug::fovReject.fetch_add(1);else debug::accepted.fetch_add(1);}}consider(best,s,reinterpret_cast<std::uintptr_t>(a.player),float(hp),origin,forward,v,last);}}
 debug::nearestAngle.store(nearest);debug::nearestDistance.store(nearestRange);last=best.valid?best.player:0;return best;
}
'''+s[end:]
s=rep(s,'inline void cameraTick(void* self,ExecuteTime time,const void* method){','inline void cameraTick(void* self,ExecuteTime time,const void* method){debug::cameraCalls.fetch_add(1);')
s=rep(s,'if(!ed::isUnityMainCallback()||!g_functionsValid)return;auto c=configuration();auto s=c.normal;if(!gate(c,s,g_normalState))return;','if(!ed::isUnityMainCallback()){debug::normalReason.store(4);return;}if(!g_functionsValid){debug::normalReason.store(5);return;}auto c=configuration();auto s=c.normal;if(!gate(c,s,g_normalState)){debug::normalReason.store(!s.enabled?1:(c.pauseInMenu&&g_menuOpen.load()?2:3));return;}')
s=rep(s,'{g_normalState.store(3);return;}if(s.onAttack&&!attacking(local.weaponPart)){g_normalState.store(4);return;}','{debug::normalReason.store(7);g_normalState.store(3);return;}if(s.onAttack&&!attacking(local.weaponPart)){debug::normalReason.store(8);g_normalState.store(4);return;}')
s=rep(s,'origin,forward)){g_normalState.store(3);return;}auto target=select','origin,forward)){debug::normalReason.store(9);g_normalState.store(3);return;}auto target=select')
s=rep(s,'if(!target.valid){g_normalState.store(5);return;}','if(!target.valid){debug::normalReason.store(10);g_normalState.store(5);return;}')
s=rep(s,'if(!std::isfinite(next.pitch)||!std::isfinite(next.yaw))return;','if(!std::isfinite(next.pitch)||!std::isfinite(next.yaw)){debug::normalReason.store(14);return;}')
s=rep(s,'g_normalApplications.fetch_add(1);g_normalState.store(6);','g_normalApplications.fetch_add(1);debug::normalReason.store(11);g_normalState.store(6);')
s=rep(s,'inline void emitRays(void* self,void* ctx,Ray* rays,std::uint64_t span){','inline void emitRays(void* self,void* ctx,Ray* rays,std::uint64_t span){debug::silentCalls.fetch_add(1);')
s=rep(s,'auto c=configuration();auto s=c.silent;int count=static_cast<std::int32_t>(span);','auto c=configuration();auto s=c.silent;int count=static_cast<std::int32_t>(span);\n debug::silentReason.store(!s.enabled?1:(!ed::isUnityMainCallback()?4:(!g_functionsValid?5:(!rays||!stackSpan(rays,count)?12:(c.pauseInMenu&&g_menuOpen.load()?2:(!s.bodies?3:0))))));')
s=rep(s,'g_silentApplications.fetch_add(changed);g_silentState.store(changed?6:7);','g_silentApplications.fetch_add(changed);debug::silentReason.store(changed?11:12);g_silentState.store(changed?6:7);')
s=rep(s,'else g_silentState.store(5);}else g_silentState.store(3);','else{debug::silentReason.store(10);g_silentState.store(5);}}else{debug::silentReason.store(9);g_silentState.store(3);}')
s=rep(s,'}else g_silentState.store(3);','}else{debug::silentReason.store(13);g_silentState.store(3);}')
s=rep(s,'actual!=expected)return false;','actual!=expected){static thread_local Clock::time_point last{};auto now=Clock::now();if(now-last>std::chrono::seconds(5)){last=now;debug::log("signature mismatch rva=0x%llx word=%u got=%08x expected=%08x",(unsigned long long)(addr-ed::g_runtime.unityBase),i-1,actual,expected);}return false;}')
s=rep(s,'if(g_normalHookReady.load())return true;void* klass=ed::classFromSlot','bool trace=debug::due(debug::cameraAttempts);if(g_normalHookReady.load())return true;debug::normalInstallReason.store(3);void* klass=ed::classFromSlot')
s=rep(s,'if(!klass)return false;auto target=','if(!klass){debug::normalInstallReason.store(2);if(trace)debug::log("camera class missing");return false;}auto target=')
s=rep(s,'std::uintptr_t direct=0,virt=0;if(!ed::readValue','std::uintptr_t direct=0,virt=0;if(trace){field(reinterpret_cast<void*>(info),0,direct);field(reinterpret_cast<void*>(info),8,virt);debug::log("camera slot index=%u exactTarget=1 methodInfoMatches=%d directRva=0x%llx virtualRva=0x%llx",i,direct==target||virt==target,(unsigned long long)(direct-ed::g_runtime.unityBase),(unsigned long long)(virt-ed::g_runtime.unityBase));}if(!ed::readValue')
s=rep(s,'if(syscall(__NR_process_vm_writev,getpid(),&src,1,&dst,1,0)!=8)return false;','if(syscall(__NR_process_vm_writev,getpid(),&src,1,&dst,1,0)!=8){debug::normalInstallReason.store(4);if(trace)debug::log("camera slot write-probe denied errno=%d",errno);return false;}')
s=rep(s,'g_normalHookReady.store(true);','debug::normalInstallReason.store(1);g_normalHookReady.store(true);')
s=rep(s,'if(g_silentHookReady.load())return true;auto site=','bool trace=debug::due(debug::silentAttempts);if(g_silentHookReady.load())return true;auto site=')
s=rep(s,'if(!ed::readValue(reinterpret_cast<void*>(site),word)||word!=0x97fffd71)return false;','if(!ed::readValue(reinterpret_cast<void*>(site),word)||word!=0x97fffd71){debug::silentInstallReason.store(5);if(trace)debug::log("silent callsite mismatch got=%08x expected=97fffd71",word);return false;}')
s=rep(s,'if(!pageSize)return false;','if(!pageSize){debug::silentInstallReason.store(6);return false;}')
s=rep(s,'if(relay==MAP_FAILED)return false;','if(relay==MAP_FAILED){debug::silentInstallReason.store(7);if(trace)debug::log("silent relay allocation failed errno=%d",errno);return false;}')
s=rep(s,'if(mprotect(relay,pageSize,PROT_READ|PROT_EXEC)){munmap','if(mprotect(relay,pageSize,PROT_READ|PROT_EXEC)){debug::silentInstallReason.store(8);if(trace)debug::log("silent relay RX denied errno=%d",errno);munmap')
s=rep(s,'if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_WRITE|PROT_EXEC)){munmap','if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_WRITE|PROT_EXEC)){debug::silentInstallReason.store(9);if(trace)debug::log("silent text RWX denied errno=%d",errno);munmap')
s=rep(s,'if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC)){__atomic_store_n','if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC)){debug::silentInstallReason.store(10);if(trace)debug::log("silent restore RX denied errno=%d",errno);__atomic_store_n')
s=rep(s,'g_relay=relay;g_silentHookReady.store(true);','g_relay=relay;debug::silentInstallReason.store(1);g_silentHookReady.store(true);')
start=s.index('inline void onPlayer(');s=s[:start]+'''inline void onPlayer(void* p){
 debug::playerCalls.fetch_add(1);if(!g_active.load(std::memory_order_acquire)){debug::inactive.fetch_add(1);return;}if(!ed::g_runtime.ready){debug::runtimeNotReady.fetch_add(1);return;}if(!ed::isUnityMainCallback()){debug::wrongThread.fetch_add(1);return;}
 debug::mainCalls.fetch_add(1);debug::mainBase.store(ed::g_runtime.unityBase);debug::callbackTid.store(int(syscall(SYS_gettid)));debug::lastPlayer.store(reinterpret_cast<std::uintptr_t>(p));
 if(!g_functionsValid){debug::validationAttempts.fetch_add(1);g_functionsValid=validateFunctions();if(!g_functionsValid){debug::validationFailures.fetch_add(1);g_normalState.store(7);g_silentState.store(7);return;}debug::log("function signatures validated on UnityMain");}
 auto now=Clock::now();auto c=configuration();if(now-g_lastInstall>std::chrono::seconds(1)){g_lastInstall=now;if(c.normal.enabled&&!installCamera())g_normalState.store(1);if(c.silent.enabled&&!installSilent())g_silentState.store(1);}capture(p);
}
}
'''
s=rep(s,'inline bool localAlive(void* p,ed::PlayerSnapshot& s,void*& world){std::uint8_t local=0;world=object(p,offsets::field::mfc::kWorld);return p&&ed::objectClass(p)==ed::g_runtime.playerClass&&field(p,offsets::field::mfc::kLocalOwned,local)&&local==1&&world&&ed::readPlayerSnapshot(p,s)&&s.localOwned&&s.statsValid&&s.hp>0&&ed::validTeam(s.team)&&s.weaponPart&&s.activeWeapon;}', '''inline bool localAlive(void* p,ed::PlayerSnapshot& s,void*& world){std::uint8_t local=0;world=object(p,offsets::field::mfc::kWorld);int mask=0;if(p)mask|=1;if(p&&ed::objectClass(p)==ed::g_runtime.playerClass)mask|=2;if(field(p,offsets::field::mfc::kLocalOwned,local))mask|=4;if(local==1)mask|=8;if(world)mask|=16;if((mask&31)==31&&ed::readPlayerSnapshot(p,s))mask|=32;if(s.localOwned)mask|=64;if(s.statsValid)mask|=128;if(s.hp>0)mask|=256;if(ed::validTeam(s.team))mask|=512;if(s.weaponPart)mask|=1024;if(s.activeWeapon)mask|=2048;debug::localMask.store(mask);debug::localRaw.store(local);debug::localHp.store(s.hp);debug::localTeam.store(s.team);return mask==4095;}''')
s=rep(s,'inline bool attacking(void* part){std::uint8_t raw=0;return field(part,offsets::field::nrk::kWeaponState,raw)&&raw==aim_offsets::kAttackState;}','inline bool attacking(void* part){std::uint8_t raw=0;bool ok=field(part,offsets::field::nrk::kWeaponState,raw);debug::attackRaw.store(ok?int(raw):-1);return ok&&raw==aim_offsets::kAttackState;}')
p.write_text(s)
p=r/'jni/main.cpp';s=p.read_text();s=rep(s,'    int stagnantChecks = 0;','    int stagnantChecks = 0;\n    auto lastAimDebug = std::chrono::steady_clock::time_point{};');s=rep(s,'        std::this_thread::sleep_for(std::chrono::seconds(2));','        auto aimNow = std::chrono::steady_clock::now();\n        if (aimNow - lastAimDebug >= std::chrono::seconds(5)) { lastAimDebug = aimNow; lemming::aim::debug::report(); }\n        std::this_thread::sleep_for(std::chrono::seconds(2));');p.write_text(s)
p=r/'jni/game_offsets.hpp';s=p.read_text();s=rep(s,'v9-angle-silent-aim','v10-aim-debug');p.write_text(s)
p=r/'jni/aim_ui.hpp';s=p.read_text();s=rep(s,'uiText(d,ImVec2(x+410*s,y+602*s),','if(cfg.enabled&&state==0)state=1;uiText(d,ImVec2(x+410*s,y+602*s),');p.write_text(s)
print('BACKUP',b)
print('Aim debug instrumentation applied')
