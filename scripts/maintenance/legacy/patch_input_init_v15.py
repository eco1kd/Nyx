from pathlib import Path
import os,sys
changes={}
def edit(p,a,b):
 s=changes.get(p,Path(p).read_text());assert s.count(a)==1,(p,a[:80],s.count(a));changes[p]=s.replace(a,b)
p='jni/src/features/aim/core.hpp'
edit(p,'#include "features/aim/automation.hpp"','#include "features/aim/automation.hpp"\n#include "features/aim/input_init.hpp"')
edit(p,'inline std::atomic<int> g_autoStage{0};','inline std::atomic<int> g_autoStage{0},g_inputInstallReason{input_init::NotAttempted};')
p='jni/src/features/aim/runtime.hpp'
edit(p,'inline void onPlayer(void* p){','inline input_init::Retry g_inputRetry{};\ninline void onPlayer(void* p){')
edit(p,'if(c.autoFire.enabled&&p==localPlayer())installInput(p);','')
edit(p,'}prepareAssists(c.normal.rcs,','}auto* local=localPlayer();if(g_inputRetry.due(autoNow(),c.autoFire.enabled,g_inputHookReady.load(),reinterpret_cast<std::uintptr_t>(local)))installInput(local);else if(c.autoFire.enabled&&!g_inputHookReady.load()&&!local)g_inputInstallReason.store(input_init::NoLocal);prepareAssists(c.normal.rcs,')
p='jni/src/features/aim/automation_runtime.hpp'
s=Path(p).read_text();a=s.index('inline void* component(');b=s.index('inline bool autoCameraValid',a)
component='''inline void* component(void* p,const char* name,input_init::ComponentStats* stats=nullptr){
 input_init::ComponentStats count{};auto finish=[&](void* c){if(stats)*stats=count;return c;};
 auto* world=object(p,0x18);auto* container=object(p,0x20);if(!p||!world||!named(container,"wmz"))return finish(nullptr);
 auto accept=[&](void* c){if(!c)return false;++count.objects;if(!named(c,name))return false;if(object(c,0x18)!=p||object(c,0x20)!=world){++count.foreign;return false;}return true;};
 for(std::size_t off:{0x10u,0x18u,0x20u,0x28u,0x30u,0x38u}){auto* arr=object(container,off);std::uintptr_t n=0;if(!field(arr,0x18,n)||n>256)continue;++count.arrays;for(std::uintptr_t i=0;i<n;++i){auto* c=object(arr,0x20+8*i);if(accept(c))return finish(c);}}
 // Dictionary<Type,chx> layout verified in matching il2cpp.h: entries+18, count+20, Entry stride18/value10.
 auto* dictionary=object(container,0x40);auto* entries=object(dictionary,0x18);std::int32_t n=0;std::uintptr_t capacity=0;
 if(field(dictionary,0x20,n)&&n>=0&&n<=256&&field(entries,0x18,capacity)&&capacity>=unsigned(n)&&capacity<=512){count.dictionary=true;for(int i=0;i<n;++i){std::int32_t hash=-1;auto off=0x20+std::size_t(i)*0x18;if(!field(entries,off,hash)||hash<0)continue;++count.entries;auto* c=object(entries,off+0x10);if(accept(c))return finish(c);}}
 return finish(nullptr);
}
'''
changes[p]=s[:a]+component+s[b:]
s=changes[p];a=s.index('inline bool installInput(')
install='''inline bool installInput(void* p){
 if(g_inputHookReady.load()){g_inputInstallReason.store(input_init::Ready);return true;}
 auto attempt=debug::inputAttempts.fetch_add(1)+1;
 auto result=[&](int code){int previous=g_inputInstallReason.exchange(code);if(previous!=code||attempt<=3||attempt%10==0)debug::log("input install attempt=%llu reason=%s local=%p",(unsigned long long)attempt,input_init::reasonText(code),p);return code==input_init::Ready;};
 if(!ed::isUnityMainCallback())return result(input_init::WrongThread);
 if(!p||p!=localPlayer())return result(input_init::NoLocal);
 if(!ed::g_runtime.ready||!g_functionsValid)return result(input_init::RuntimeNotReady);
#if defined(__aarch64__)
 ed::PlayerSnapshot snapshot{};void* world=nullptr;if(!localAlive(p,snapshot,world))return result(input_init::InvalidLocal);
 input_init::ComponentStats stats{};auto* input=component(p,"uys",&stats);if(!input){if(attempt<=3||attempt%10==0)debug::log("input component search arrays=%u dictionary=%d entries=%u objects=%u foreign=%u",stats.arrays,stats.dictionary,stats.entries,stats.objects,stats.foreign);return result(input_init::ComponentMissing);}
 if(!scopeEnumProfile()&&(attempt<=3||attempt%10==0))debug::log("scope enum unavailable: Auto scope paused, plain Triggerbot/Angles remain independent");auto b=ed::g_runtime.unityBase;
 if(!bytesMatch(b+aim_offsets::kInputTick,{0xd10183ff,0xfd0013e8,0xf90017fe,0xa9035ff8})||!bytesMatch(b+aim_offsets::kCommandAttack,{0x3900c801,0xd65f03c0})||!bytesMatch(b+aim_offsets::kCommandScope,{0x3900cc01,0xd65f03c0})||!bytesMatch(b+aim_offsets::kCommandScopeHold,{0x3900d001,0xd65f03c0})||!bytesMatch(b+aim_offsets::kCommandLook,{0xd10103ff,0xa90257fe,0xa9034ff4,0x90018a35}))return result(input_init::SignatureRejected);
 auto* klass=ed::objectClass(input);if(!klass||!ed::nameEquals(ed::className(klass),"uys"))return result(input_init::ClassRejected);auto target=b+aim_offsets::kInputTick;
 for(unsigned i=0;i<128;++i){auto* slot=reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(klass)+ed::kRuntimeVtableOffset+i*ed::kVirtualInvokeDataSize);std::uintptr_t fn=0,metadata=0;if(!ed::readValue(slot,fn)||!ed::readValue(slot+1,metadata)||!hook_support::cameraSlotMatches(fn,target,metadata,ed::readable(reinterpret_cast<void*>(metadata),16),b+offsets::elf::kTextBegin,b+offsets::elf::kTextEnd))continue;ed::refreshMaps(true);if(!ed::writable(slot,8))return result(input_init::SlotReadOnly);std::uint64_t probe=fn;iovec src{&probe,8},dst{slot,8};if(syscall(__NR_process_vm_writev,getpid(),&src,1,&dst,1,0)!=8)return result(input_init::WriteProbeFailed);
 g_commandAttack=reinterpret_cast<CommandBoolFn>(b+aim_offsets::kCommandAttack);g_commandScope=reinterpret_cast<CommandBoolFn>(b+aim_offsets::kCommandScope);g_commandScopeHold=reinterpret_cast<CommandBoolFn>(b+aim_offsets::kCommandScopeHold);g_commandLook=reinterpret_cast<CommandLookFn>(b+aim_offsets::kCommandLook);g_inputOriginal.store(reinterpret_cast<CameraTickFn>(fn),std::memory_order_release);auto replacement=reinterpret_cast<std::uintptr_t>(&inputTick);__atomic_store_n(slot,replacement,__ATOMIC_RELEASE);std::uintptr_t current=0;if(!ed::readValue(slot,current)||current!=replacement){__atomic_store_n(slot,fn,__ATOMIC_RELEASE);g_inputOriginal.store(nullptr);return result(input_init::ReadbackFailed);}g_inputHookReady.store(true);debug::log("input hook ready uys slot=%u; attack/scope/look signatures verified",i);return result(input_init::Ready);}
 return result(input_init::SlotMissing);
#else
 return result(input_init::UnsupportedArchitecture);
#endif
}
'''
changes[p]=s[:a]+install
p='jni/src/features/aim/debug.hpp'
edit(p,'inline Count inputCalls{0},','inline Count inputAttempts{0};\ninline Count inputCalls{0},')
edit(p,'hook=%d stage=%s','hook=%d installAttempts=%llu installReason=%s stage=%s')
edit(p,'g_inputHookReady.load(),automation::stageText','g_inputHookReady.load(),(unsigned long long)inputAttempts.load(),input_init::reasonText(g_inputInstallReason.load()),automation::stageText')
p='jni/src/ui/pages/aim.hpp'
edit(p,'"Automation pending / rejected; Angles remain active"','(!g_inputHookReady.load()?input_init::reasonText(g_inputInstallReason.load()):"Shot / raycast profiles pending; Angles active")')
p='scripts/test.sh';edit(p,' run_case prediction tests/aim/prediction_test.cpp -ldl -lpthread',' run_case input_init tests/aim/input_init_test.cpp -ldl -lpthread\n run_case prediction tests/aim/prediction_test.cpp -ldl -lpthread')
for p in ['jni/src/game/offsets.hpp','jni/src/features/aim/debug.hpp','tests/aim/debug_smoke.cpp']:
 s=changes.get(p,Path(p).read_text());assert 'v14-prediction-fallback' in s;changes[p]=s.replace('v14-prediction-fallback','v15-local-input-init')
print('Preflight PASS',len(changes),'files')
if '--check' in sys.argv:sys.exit(0)
for p,s in changes.items():
 target=Path(p);mode=target.stat().st_mode;tmp=target.with_name(target.name+'.v15tmp');tmp.write_text(s);tmp.chmod(mode);os.replace(tmp,target)
print('Applied local-input scheduling, verified component search and installation reasons')
