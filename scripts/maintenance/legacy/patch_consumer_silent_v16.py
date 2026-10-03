#!/usr/bin/env python3
from pathlib import Path
import os,sys
ROOT=Path.cwd();changes={}
def change(path,old,new):
 p=ROOT/path;s=changes.get(p,p.read_text());assert s.count(old)==1,(path,old[:90],s.count(old));changes[p]=s.replace(old,new)
def update(path,fn):
 p=ROOT/path;changes[p]=fn(changes.get(p,p.read_text()))
change('jni/src/features/aim/core.hpp','int bone=-1;};\ninline void consider','int bone=-1;HitVolume volume{};};\ninline void consider')
change('jni/src/features/aim/core.hpp','best={true,player,p[i],angle,distance,score,v.bone};','best={true,player,p[i],angle,distance,score,v.bone,v};')
change('jni/src/features/aim/core.hpp','inline std::atomic<bool> g_inputHookReady{false};','inline std::atomic<bool> g_inputHookReady{false},g_autoScopeReady{false};')
def patch_runtime(s):
 s=s.replace('#include "features/aim/prediction.hpp"','#include "features/aim/prediction.hpp"\n#include "features/aim/silent_accuracy.hpp"\n#include "features/aim/native_pair.hpp"',1)
 marker='inline bool transformPose(';assert s.count(marker)==1;s=s.replace(marker,'#include "features/aim/silent_runtime.hpp"\n'+marker,1)
 old='if(target.valid){unsigned changed=0;for(int i=0;i<count;++i){Ray r{};if(!ed::readValue(rays+i,r)||!finite(r.origin))continue;auto direction=normalized(sub(predictPoint(target,s,r.origin,false),r.origin));if(length(direction)<.9f)continue;std::memcpy(&rays[i].direction,&direction,sizeof(direction));++changed;}g_silentApplications.fetch_add(changed);debug::silentReason.store(changed?11:12);g_silentState.store(changed?6:7);}'
 new='if(target.valid){unsigned changed=0;Vec3 point{};std::int32_t colliderId=0;bool resolved=silentLivePoint(target,s,ray.origin,forward,point,colliderId);if(resolved)for(int i=0;i<count;++i){Ray r{};if(!ed::readValue(rays+i,r)||!silent_accuracy::inFov(r.origin,point,forward,s)||(s.visibleCheck&&!silentHitsBone(r.origin,point,colliderId)))continue;auto direction=normalized(sub(point,r.origin));if(length(direction)<.9f)continue;std::memcpy(&rays[i].direction,&direction,sizeof(direction));++changed;}g_silentApplications.fetch_add(changed);debug::silentReason.store(changed?11:18);g_silentState.store(changed?6:5);}'
 assert s.count(old)==1;s=s.replace(old,new,1)
 old='prepareAssists(c.normal.rcs,c.normal.visibleCheck||c.autoFire.enabled);capture(p,(c.normal.visibleCheck||c.autoFire.enabled)&&g_visibilityReady.load());'
 new='bool visibility=c.normal.visibleCheck||c.silent.enabled||c.autoFire.enabled;prepareAssists(c.normal.rcs,visibility);capture(p,visibility&&g_visibilityReady.load());'
 assert s.count(old)==1;return s.replace(old,new,1)
update('jni/src/features/aim/runtime.hpp',patch_runtime)
PHASE=r'''// Commands are leased only between early/late consumers of one producer generation.
struct CommandPhaseLease {void* self=nullptr;void* cmd=nullptr;void* player=nullptr;void* world=nullptr;std::uint32_t token=0;CommandLook look{};bool lookValid=false,lookOwned=false;};inline CommandPhaseLease g_phaseLease{};
inline void discardCommandPhase(){g_ownedAttack=g_ownedScope=g_ownedScopeHold=false;g_ownedCommand=nullptr;g_phaseLease={};}
inline bool beginCommandPhase(void* self,void* cmd){
 if(!ed::isUnityMainCallback()||!named(self,"ltc")||object(self,0x18)!=localPlayer()||!named(cmd,"owv"))return false;
 CommandPhaseLease phase{};phase.self=self;phase.cmd=cmd;phase.player=object(self,0x18);phase.world=object(self,0x20);
 if(!phase.world||!field(cmd,0x10,phase.token)||!ed::writable(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(cmd)+0x32),3))return false;
 phase.lookValid=field(cmd,0x1c,phase.look)&&phase.look.has<=1;g_phaseLease=phase;return true;
}
inline void finishCommandPhase(void* self){
 if(!g_phaseLease.cmd||self!=g_phaseLease.self||!ed::isUnityMainCallback())return;
 auto phase=g_phaseLease;std::uint32_t token=0;
 if(named(phase.cmd,"owv")&&field(phase.cmd,0x10,token)&&token==phase.token&&object(self,0x18)==phase.player&&object(self,0x20)==phase.world){
  releaseOwnedCommand();
  if(phase.lookOwned&&phase.lookValid&&ed::writable(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(phase.cmd)+0x1c),12))std::memcpy(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(phase.cmd)+0x1c),&phase.look,12);
  debug::commandCompleted.fetch_add(1);
 }
 discardCommandPhase();
}
'''
def patch_automation(s):
 s=s.replace('inline std::atomic<CameraTickFn> g_inputOriginal{nullptr};\n','',1)
 marker='inline void autoShotObserved(';assert s.count(marker)==1;s=s.replace(marker,PHASE+marker,1)
 start=s.index('inline void inputTick(');body=s.index(' auto c=configuration();',start);finish=s.index('inline bool installInput(',body)
 prefix='inline void processAutomation(void* self,void* cmd){\n if(!ed::isUnityMainCallback()||object(self,0x18)!=localPlayer())return;\n'
 suffix=s[body:finish].replace('options.returnCamera=options.returnCamera&&c.normal.enabled;','',1)
 suffix=suffix.replace('auto* p=object(self,0x18);auto* cmd=object(self,0x40);std::uint8_t mode=0,','auto* p=object(self,0x18);std::uint8_t ',1)
 old='p==localPlayer()&&named(self,"uys")&&localAlive(p,local,world)&&object(self,0x20)==world&&named(cmd,"owv")&&field(self,0x48,mode)&&mode==101'
 new='p==localPlayer()&&named(self,"ltc")&&localAlive(p,local,world)&&object(self,0x20)==world&&object(self,0x50)==local.weaponPart&&named(cmd,"owv")'
 assert old in suffix;suffix=suffix.replace(old,new,1)
 old='auto* camera=identity?component(p,"ios"):nullptr;int kind=data?autoGunKind(local.activeWeapon):0;'
 new=old+'options.autoScope=options.autoScope&&g_autoScopeReady.load()&&kind==2;debug::autoSafetyMask.store((identity?1:0)|(data?2:0)|(kind?4:0)|(ammo?8:0)|(g_visibilityReady.load()?16:0)|(g_autoScopeReady.load()?32:0));'
 assert old in suffix;suffix=suffix.replace(old,new,1)
 old='&&std::isfinite(time.time)&&std::isfinite(time.delta)&&time.delta>0&&time.delta<=.1f';assert old in suffix;suffix=suffix.replace(old,'',1)
 old='if(g_commandLook&&ed::writable(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(cmd)+0x1c),12))g_commandLook(cmd,CommandLook{1,{0,0,0},desired.pitch,desired.yaw},nullptr);'
 new='if(g_commandLook&&g_phaseLease.cmd==cmd&&g_phaseLease.lookValid&&ed::writable(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(cmd)+0x1c),12)){g_commandLook(cmd,CommandLook{1,{0,0,0},desired.pitch,desired.yaw},nullptr);g_phaseLease.lookOwned=true;}'
 assert old in suffix;suffix=suffix.replace(old,new,1).replace('if(!safe||o.manual||!options.enabled||','if(!safe||o.manual||!c.autoFire.enabled||',1)
 s=s[:start]+prefix+suffix+'#include "features/aim/command_phase_runtime.hpp"\n'
 return s.replace('g_scopeNone=values[0];g_scopeIn=values[1];g_scopeSwitch=values[2];g_scopeReload=values[3];','g_scopeNone=values[0];g_scopeIn=values[1];g_scopeSwitch=values[2];g_scopeReload=values[3];g_autoScopeReady.store(true);',1)
update('jni/src/features/aim/automation_runtime.hpp',patch_automation)
change('jni/src/features/aim/offsets.hpp','inline constexpr std::uint8_t kAttackState=1;','// Verified ltc early/late command consumer pair, v16.\ninline constexpr std::uintptr_t kCommandGate=0x94A0990,kCommandGateSite=0x94A3354;\ninline constexpr std::uintptr_t kCommandEnd=0x949ECD8,kCommandEndSite=0x94A6A1C;\ninline constexpr std::uint8_t kAttackState=1;')
change('jni/src/features/aim/input_init.hpp','UnsupportedArchitecture,RuntimeNotReady};','UnsupportedArchitecture,RuntimeNotReady,CallsiteRejected,RelayAllocationFailed,RelayProtectionFailed,TextProtectionFailed,RestoreProtectionFailed,PairReadbackFailed};')
change('jni/src/features/aim/input_init.hpp','case Ready:return "ready";','case CallsiteRejected:return "command-callsite-changed";case RelayAllocationFailed:return "command-relay-allocation-failed";case RelayProtectionFailed:return "command-relay-RX-failed";case TextProtectionFailed:return "command-text-protection-failed";case RestoreProtectionFailed:return "command-restore-RX-failed";case PairReadbackFailed:return "command-pair-verify-or-rollback-failed";case Ready:return "ready";')
def patch_debug(s):
 s=s.replace('inline Count inputAttempts{0};','inline Count inputAttempts{0},commandCompleted{0},commandRejected{0};\ninline Count silentLiveRefresh{0},silentCenterFallback{0},silentPointRejected{0};\ninline std::atomic<int> silentBone{-1},autoSafetyMask{0};\ninline std::atomic<float> silentLeadCm{0};',1).replace('v15-local-input-init','v16-command-phase-hitbox',1)
 s=s.replace('case 17:return "automation-camera-owner";','case 17:return "automation-camera-owner";case 18:return "live-silent-hitbox-rejected";',1)
 s=s.replace('scopePulses=%llu timeouts=%llu (shots != damage)','scopePulses=%llu timeouts=%llu phaseEnds=%llu gateRejects=%llu safety=%02x scopeReady=%d silentPoint[bone=%d leadCm=%.2f live=%llu centerFallback=%llu rejected=%llu] (shots != damage)',1)
 old='(unsigned long long)autoScopePulses.load(),(unsigned long long)autoTimeouts.load());}'
 new='(unsigned long long)autoScopePulses.load(),(unsigned long long)autoTimeouts.load(),(unsigned long long)commandCompleted.load(),(unsigned long long)commandRejected.load(),autoSafetyMask.load(),g_autoScopeReady.load(),silentBone.load(),silentLeadCm.load(),(unsigned long long)silentLiveRefresh.load(),(unsigned long long)silentCenterFallback.load(),(unsigned long long)silentPointRejected.load());}'
 assert old in s;return s.replace(old,new,1)
update('jni/src/features/aim/debug.hpp',patch_debug)
def patch_test(s):
 s=s.replace('static int originals=0;static void original(void*,rt::ExecuteTime,const void*){++originals;}','static int originals=0;static bool original(void*,void*,const void*){++originals;return true;}',1).replace('rt::g_inputOriginal.store(original);','rt::g_commandGateOriginal.store(original);',1).replace('rt::inputTick(&input,{1,.016f},nullptr);','assert(rt::commandGate(&input,&cmd,nullptr));',1).replace('rt::inputTick(&input,{2,.016f},nullptr);','assert(rt::commandGate(&input,&cmd,nullptr));',1)
 return s
update('tests/aim/automation_runtime_test.cpp',patch_test)
change('scripts/test.sh',' run_case automation_runtime tests/aim/automation_runtime_test.cpp -ldl -lpthread',' run_case automation_runtime tests/aim/automation_runtime_test.cpp -ldl -lpthread\n run_case command_phase tests/aim/command_phase_test.cpp -ldl -lpthread\n run_case silent_accuracy tests/aim/silent_accuracy_test.cpp -ldl -lpthread\n run_case native_pair tests/aim/native_pair_test.cpp -ldl -lpthread')
print('Preflight PASS',len(changes),'files')
if '--apply' in sys.argv:
 for p,s in changes.items():
  mode=p.stat().st_mode;t=p.with_name(p.name+'.v16-tmp');t.write_text(s);os.chmod(t,mode);os.replace(t,p)
 print('Applied once; permissions preserved')
