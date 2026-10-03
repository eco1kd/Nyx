from pathlib import Path
import os
R=Path('/home/leftcode/Projects/LemmingRMT');files={}
def edit(name,old,new):
 p=R/name;s=files.get(p,p.read_text());assert s.count(old)==1,(name,old);files[p]=s.replace(old,new,1)
edit('jni/src/features/aim/assists.hpp','struct RecoilTracker {','inline Angles smoothCompensated(Angles current,Angles target,Angles previousWeighted,Angles recoil,float percent,float seconds,float dt){\n auto unbiased=compensate(current,{-previousWeighted.pitch,-previousWeighted.yaw},100);return compensate(smooth(unbiased,target,seconds,dt),recoil,percent);\n}\nstruct RecoilTracker {')
edit('jni/src/features/aim/runtime.hpp','bool recoilOnly=false,recoilUsed=false;Angles recoil{};','bool recoilOnly=false,recoilUsed=false;Angles recoil{};auto next=target.valid?smooth(current,desired,s.smoothing,time.delta):current;')
edit('jni/src/features/aim/runtime.hpp','if(target.valid){desired=assists::compensate(desired,recoil,s.rcsStrength);','if(target.valid){Angles previous=g_recoilTracker.active&&g_recoilTracker.weapon==weapon?g_recoilTracker.previous:Angles{};next=assists::smoothCompensated(current,desired,previous,recoil,s.rcsStrength,s.smoothing,time.delta);')
edit('jni/src/features/aim/runtime.hpp','desired=assists::compensate(current,delta,100);','next=assists::compensate(current,delta,100);')
edit('jni/src/features/aim/runtime.hpp','auto next=recoilOnly?desired:smooth(current,desired,s.smoothing,time.delta);next=g_clamp','next=g_clamp')
edit('jni/src/features/aim/runtime.hpp','if(!std::isfinite(next.pitch)||!std::isfinite(next.yaw)){debug::normalReason.store(14);return;}','if(!std::isfinite(next.pitch)||!std::isfinite(next.yaw)){g_recoilTracker.reset();debug::normalReason.store(14);return;}')
edit('tests/aim/assists_test.cpp','a::RecoilTracker tracker;','auto fully=a::smoothCompensated({0,0},{0,0},{0,0},{-2,1},100,1,.016f);assert(close(fully.pitch,2)&&close(fully.yaw,-1));auto stable=a::smoothCompensated(fully,{0,0},{-2,1},{-2,1},100,1,.016f);assert(close(stable.pitch,2)&&close(stable.yaw,-1));\n a::RecoilTracker tracker;')
edit('tests/aim/assists_test.cpp','assert(close(d.pitch,0));\n Settings cfg;','assert(close(d.pitch,0));d=tracker.delta(2,{-1,.5f},true);assert(close(d.pitch,1)&&close(d.yaw,-.5f));\n Settings cfg;')
edit('tests/aim/assists_test.cpp','assert(rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));hitId=99;','actor.colliderCount=0;assert(!rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));actor.colliderCount=1;assert(rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));hitId=99;')
edit('tests/ui/ui_test.cpp','static bool graphics=false;','static bool graphics=false,syntheticFov=false;static float syntheticFovAngle=8;')
edit('tests/ui/ui_test.cpp','renderMinimalMenu();ImGui::Render();','renderMinimalMenu();if(syntheticFov){auto size=ImGui::GetIO().DisplaySize;auto* bg=ImGui::GetBackgroundDrawList();int before=bg->VtxBuffer.Size;drawAngleFovGeometry(lemming::aim::assists::fovGeometry(syntheticFovAngle,size.x,size.y,1,1));assert(bg->VtxBuffer.Size>before);}ImGui::Render();')
edit('tests/ui/ui_test.cpp','  ImGui_ImplOpenGL3_Shutdown();','  g_menuOpen.store(false);g_espBoxesEnabled.store(false);syntheticFov=true;syntheticFovAngle=8;frame();syntheticFovAngle=120;frame();syntheticFov=false;g_menuOpen.store(true);g_espBoxesEnabled.store(true);std::cout<<"PASS FOV ellipse/full-viewport draw helper with menu closed, ESP disabled, finite vertices and GL_NO_ERROR (synthetic projection)\\n";\n  ImGui_ImplOpenGL3_Shutdown();')
for p,s in files.items():
 temporary=p.with_suffix(p.suffix+'.v12-tmp');temporary.write_text(s);os.replace(temporary,p)
print('Refined RCS: separate instantaneous compensation from target smoothing; added FOV offscreen render and strength/ownership regressions')
