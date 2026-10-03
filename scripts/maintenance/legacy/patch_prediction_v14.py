from pathlib import Path
import os,sys
edits={}
def load(p):
 if p not in edits: edits[p]=Path(p).read_text()
 return edits[p]
def change(p,a,b):
 s=load(p);assert s.count(a)==1,(p,a[:70],s.count(a));edits[p]=s.replace(a,b)
p='jni/src/features/aim/core.hpp'
change(p,'visibleCheck=false;','visibleCheck=false,prediction=true;')
change(p,'rcsStrength=100;','rcsStrength=100,predictionMs=20;')
change(p,'s.rcsStrength=finiteClamp(s.rcsStrength,100,0,100);','s.rcsStrength=finiteClamp(s.rcsStrength,100,0,100);s.predictionMs=finiteClamp(s.predictionMs,20,0,100);')
p='jni/src/features/aim/runtime.hpp'
change(p,'#include "features/aim/core.hpp"','#include "features/aim/core.hpp"\n#include "features/aim/prediction.hpp"')
change(p,'Clock::time_point captured{};};','Clock::time_point captured{};prediction::Motion motion{};};')
change(p,'if(dst){*dst=a;debug::storedActors','if(dst){if(dst->player==p&&dst->world==a.world&&dst->team==a.team&&dst->hp>0)a.motion=dst->motion;{std::lock_guard<std::mutex> lock(ed::g_playerPoseMutex);for(const auto& pose:ed::g_playerPoses)if(pose.player==p&&pose.world==a.world){a.motion.update({pose.position.x,pose.position.y,pose.position.z},std::chrono::duration<double>(pose.captured.time_since_epoch()).count(),reinterpret_cast<std::uintptr_t>(a.world),a.team);break;}}*dst=a;debug::storedActors')
helper='''inline Vec3 predictPoint(const Selection& target,const Settings& s,Vec3 origin,bool angleMode){
 if(!target.valid||!s.prediction)return target.point;
 double now=std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
 for(const auto& a:g_actors)if(reinterpret_cast<std::uintptr_t>(a.player)==target.player){auto point=a.motion.lead(target.point,now,s.predictionMs,angleMode?s.smoothing:0,s.prediction);auto delta=sub(point,origin);float n=length(delta);if(!finite(point)||!std::isfinite(n)||n<.2f||n>s.maxDistance)return target.point;if(s.visibleCheck&&!visibilityAccept(origin,point,const_cast<Actor*>(&a)))return target.point;return point;}
 return target.point;
}
'''
change(p,'inline bool transformPose(',helper+'inline bool transformPose(')
change(p,'if(c.autoFire.enabled&&c.autoFire.returnCamera){g_recoilTracker.reset();return;}','if(automationOwnsCamera(self,c)){g_recoilTracker.reset();debug::normalReason.store(17);g_normalState.store(10);return;}')
change(p,'anglesFor(sub(target.point,origin)):current','anglesFor(sub(predictPoint(target,s,origin,true),origin)):current')
change(p,'auto direction=normalized(sub(target.point,r.origin));','auto direction=normalized(sub(predictPoint(target,s,r.origin,false),r.origin));')
change(p,'// Forward once; Silent direction edits are unchanged. Automation observes the completed local emission.','// Forward once; bounded prediction changes only the selected direction. Automation observes local emission.')
p='jni/src/features/aim/automation_runtime.hpp'
change(p,'inline double autoNow(){','inline double g_cameraLeaseUntil=0;inline std::uintptr_t g_cameraLeasePlayer=0,g_cameraLeaseWorld=0;\ninline double autoNow(){')
lease='''inline bool automationOwnsCamera(void* camera,const Configuration& c){return c.normal.enabled&&prediction::cameraLease(c.autoFire.enabled,c.autoFire.returnCamera,g_inputHookReady.load()&&g_silentHookReady.load()&&g_visibilityReady.load(),g_menuOpen.load(),g_savedAutoView.valid||g_autoShotSerial>g_autoStartSerial,autoNow(),g_cameraLeaseUntil,g_cameraLeasePlayer,g_cameraLeaseWorld,reinterpret_cast<std::uintptr_t>(object(camera,0x18)),reinterpret_cast<std::uintptr_t>(object(camera,0x20)));}
'''
change(p,'inline void* component(',lease+'inline void* component(')
change(p,'debug::autoShots.fetch_add(1);restoreAutoView();','debug::autoShots.fetch_add(1);auto restores=debug::autoRestores.load();restoreAutoView();if(debug::autoRestores.load()>restores)g_cameraLeaseUntil=autoNow()+std::min(configuration().autoFire.intervalMs,120.f);')
change(p,'&&(scope==g_scopeNone||scope==g_scopeIn||scope==g_scopeSwitch||scope==g_scopeReload)','&&(!options.autoScope||(g_scopeNone<16&&(scope==g_scopeNone||scope==g_scopeIn||scope==g_scopeSwitch||scope==g_scopeReload)))')
change(p,'settings.visibleCheck=true;settings.enabled=true;','settings.visibleCheck=true;settings.prediction=false;settings.enabled=true;')
change(p,'closeRequested=true;g_scopeCleanup.sent=true;','closeRequested=true;')
change(p,'g_scopeCleanup={true,actions.scopePulse,o.player','g_scopeCleanup={true,false,o.player')
change(p,'else if(g_commandLook&&ed::writable(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(cmd)+0x1c),12))g_commandLook(cmd,CommandLook{1,{0,0,0},desired.pitch,desired.yaw},nullptr);','else{g_cameraLeasePlayer=o.player;g_cameraLeaseWorld=o.world;g_cameraLeaseUntil=now+80;if(g_commandLook&&ed::writable(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(cmd)+0x1c),12))g_commandLook(cmd,CommandLook{1,{0,0,0},desired.pitch,desired.yaw},nullptr);}')
change(p,'debug::autoScopePulses.fetch_add(1);','debug::autoScopePulses.fetch_add(1);if(g_scopeCleanup.pending)g_scopeCleanup.sent=true;')
change(p,'if(actions.timedOut)debug::autoTimeouts.fetch_add(1);','if(!safe||o.manual||!options.enabled||(wasActive&&!g_autoCycle.active()&&!actions.completed))g_cameraLeaseUntil=0;\n if(actions.timedOut)debug::autoTimeouts.fetch_add(1);')
change(p,'if(!input||!scopeEnumProfile())return false;','if(!input)return false;if(!scopeEnumProfile())debug::log("scope enum unavailable: Auto scope paused, plain Triggerbot/Angles remain independent");')
p='jni/src/ui/premium.hpp'
change(p,'const char* labels[6][5]','const char* labels[6][6]')
change(p,'"Multipoints","Automation"','"Multipoints","Automation","Prediction"')
change(p,'while(n<5&&','while(n<6&&')
s=load(p);s=s.replace('icons[6][5]','icons[6][6]');edits[p]=s
p='jni/src/ui/pages/aim.hpp'
change(p,'headings[5][2]','headings[6][2]')
change(p,'{"TRIGGERBOT","CAMERA & AUTO SCOPE"}}','{"TRIGGERBOT","CAMERA & AUTO SCOPE"},{"ANGLE PREDICTION","SILENT PREDICTION"}}')
change(p,'}else if(g_subsection==4){auto& cfg=g_aimUi.autoFire;','}else if(g_subsection==5){uiToggle("Angle motion prediction",g_aimUi.normal.prediction);uiSlider("normal_prediction_ms","Extra prediction lead",g_aimUi.normal.predictionMs,0,100,"%.0f ms");ImGui::TextWrapped("Velocity + snapshot age + bounded smoothing compensation. Resets on stale data, world changes and teleports.");}else if(g_subsection==4){auto& cfg=g_aimUi.autoFire;')
change(p,'}else if(g_subsection==4){uiToggle("Back camera"','}else if(g_subsection==5){uiToggle("Silent motion prediction",g_aimUi.silent.prediction);uiSlider("silent_prediction_ms","Extra prediction lead",g_aimUi.silent.predictionMs,0,100,"%.0f ms");ImGui::TextWrapped("Maximum lead: 250 ms / 1 metre. No guessed bullet speed or packet latency. Triggerbot still requires a real present-time collider hit.");}else if(g_subsection==4){uiToggle("Back camera"')
change(p,'"Waiting for input / shot / raycast profiles"','"Automation pending / rejected; Angles remain active"')
change(p,'uiText(d,ImVec2(x+410*s,y+602*s),g_subsection==3?','if(g_subsection==5)status="Built-in bounded motion prediction; live tracking still needs a phone test";uiText(d,ImVec2(x+410*s,y+602*s),g_subsection==3?')
p='tests/ui/ui_test.cpp';s=load(p);assert s.count('page<5')==2;s=s.replace('page<5','page<6').replace('"aim-automation.ppm"','"aim-automation.ppm","aim-prediction.ppm"');edits[p]=s
p='scripts/test.sh';change(p,' run_case automation tests/aim/automation_test.cpp -ldl -lpthread',' run_case prediction tests/aim/prediction_test.cpp -ldl -lpthread\n run_case automation tests/aim/automation_test.cpp -ldl -lpthread')
for p in ['jni/src/game/offsets.hpp','jni/src/features/aim/debug.hpp','tests/aim/debug_smoke.cpp']:
 s=load(p);assert 'v13-aim-automation' in s;edits[p]=s.replace('v13-aim-automation','v14-prediction-fallback')
print('Preflight PASS',len(edits),'files')
if '--check' in sys.argv:sys.exit(0)
for p,s in edits.items():
 path=Path(p);mode=path.stat().st_mode;tmp=path.with_name(path.name+'.v14tmp');tmp.write_text(s);tmp.chmod(mode);os.replace(tmp,path)
print('Applied v14 prediction and finite camera ownership, preserving file modes')
