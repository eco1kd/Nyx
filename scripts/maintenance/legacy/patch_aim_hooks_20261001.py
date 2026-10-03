from pathlib import Path
R=Path('/home/leftcode/Projects/LemmingRMT')
assert 'v10-aim-debug' in (R/'jni/src/game/offsets.hpp').read_text(), 'This one-off patch has already been applied'
p=R/'jni/src/features/aim/runtime.hpp';s=p.read_text();assert 'methodInfoMatches=%d' in s;start=s.index('inline bool installCamera(){');end=s.index('inline void onPlayer(void* p){');before=s[:start];after=s[end:]
replacement=r'''inline bool installCamera(){
#if defined(__aarch64__)
 bool trace=debug::due(debug::cameraAttempts);if(g_normalHookReady.load())return true;
 if(!g_functionsValid){debug::normalInstallReason.store(13);return false;}
 void* klass=ed::classFromSlot(offsets::typeinfo::kCameraComponent,"ios");
 if(!klass){debug::normalInstallReason.store(2);return false;}
 auto base=ed::g_runtime.unityBase,target=base+aim_offsets::kCameraTick;debug::normalInstallReason.store(3);
 for(unsigned i=0;i<128;++i){
  auto slot=reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(klass)+ed::kRuntimeVtableOffset+i*ed::kVirtualInvokeDataSize);
  std::uintptr_t fn=0,metadata=0;if(!ed::readValue(slot,fn)||!ed::readValue(slot+1,metadata))continue;
  if(!hook_support::cameraSlotMatches(fn,target,metadata,ed::readable(reinterpret_cast<void*>(metadata),16),base+offsets::elf::kTextBegin,base+offsets::elf::kTextEnd))continue;
  ed::refreshMaps(true);if(!ed::writable(slot,8)){debug::normalInstallReason.store(4);if(trace)debug::log("camera slot not writable index=%u",i);return false;}
  std::uint64_t probe=fn;iovec src{&probe,8},dst{slot,8};
  if(syscall(__NR_process_vm_writev,getpid(),&src,1,&dst,1,0)!=8){debug::normalInstallReason.store(4);if(trace)debug::log("camera slot write-probe denied errno=%d",errno);return false;}
  g_cameraOriginal.store(reinterpret_cast<CameraTickFn>(fn),std::memory_order_release);
  auto replacement=reinterpret_cast<std::uintptr_t>(&cameraTick);__atomic_store_n(slot,replacement,__ATOMIC_RELEASE);
  std::uintptr_t current=0;if(!ed::readValue(slot,current)||current!=replacement){__atomic_store_n(slot,fn,__ATOMIC_RELEASE);g_cameraOriginal.store(nullptr,std::memory_order_release);debug::normalInstallReason.store(12);return false;}
  debug::normalInstallReason.store(1);g_normalHookReady.store(true);debug::log("camera hook ready index=%u match=exact-validated-native-pointer metadata=%p",i,reinterpret_cast<void*>(metadata));return true;
 }
#else
 debug::normalInstallReason.store(11);
#endif
 return false;
}
inline bool installSilent(){
#if defined(__aarch64__)
 bool trace=debug::due(debug::silentAttempts);if(g_silentHookReady.load())return true;
 if(!g_functionsValid){debug::silentInstallReason.store(13);return false;}
 auto site=ed::g_runtime.unityBase+aim_offsets::kShotCallsite;std::uint32_t word=0;
 if(!ed::readValue(reinterpret_cast<void*>(site),word)||word!=0x97fffd71){debug::silentInstallReason.store(5);return false;}
 long rawPage=sysconf(_SC_PAGESIZE);if(rawPage<=0||!hook_support::pageSizeValid(std::uintptr_t(rawPage))){debug::silentInstallReason.store(6);return false;}
 auto pageSize=std::uintptr_t(rawPage);hook_support::AllocationStats stats{};void* relay=hook_support::allocateNear(site,pageSize,stats);
 if(trace)debug::log("silent relay search candidates=%u attempts=%u relocated=%u errno=%d pageSize=%lu selected=%p",stats.candidates,stats.attempts,stats.relocated,stats.lastError,(unsigned long)pageSize,relay==MAP_FAILED?nullptr:relay);
 if(relay==MAP_FAILED){debug::silentInstallReason.store(7);return false;}
 std::uint32_t branch=0;if(!hook_support::branchWord(site,reinterpret_cast<std::uintptr_t>(relay),branch)){munmap(relay,pageSize);debug::silentInstallReason.store(7);return false;}
 std::uint32_t instructions[2]={0x58000051,0xd61f0220};auto replacement=reinterpret_cast<std::uintptr_t>(&emitRays);
 std::memcpy(relay,instructions,8);std::memcpy(static_cast<char*>(relay)+8,&replacement,8);__builtin___clear_cache(static_cast<char*>(relay),static_cast<char*>(relay)+16);
 if(mprotect(relay,pageSize,PROT_READ|PROT_EXEC)){debug::silentInstallReason.store(8);if(trace)debug::log("silent relay RX denied errno=%d",errno);munmap(relay,pageSize);return false;}
 auto page=site&~(pageSize-1);
 if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_WRITE|PROT_EXEC)){debug::silentInstallReason.store(9);if(trace)debug::log("silent text RWX denied errno=%d",errno);munmap(relay,pageSize);return false;}
 std::uint32_t current=0;if(!ed::readValue(reinterpret_cast<void*>(site),current)||current!=word){mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC);munmap(relay,pageSize);debug::silentInstallReason.store(5);return false;}
 __atomic_store_n(reinterpret_cast<std::uint32_t*>(site),branch,__ATOMIC_RELEASE);__builtin___clear_cache(reinterpret_cast<char*>(site),reinterpret_cast<char*>(site+4));
 if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC)){debug::silentInstallReason.store(10);if(trace)debug::log("silent restore RX denied errno=%d",errno);__atomic_store_n(reinterpret_cast<std::uint32_t*>(site),word,__ATOMIC_RELEASE);__builtin___clear_cache(reinterpret_cast<char*>(site),reinterpret_cast<char*>(site+4));mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC);return false;}
 g_relay=relay;debug::silentInstallReason.store(1);g_silentHookReady.store(true);debug::log("silent shot call-site ready relay=%p branch=%08x",relay,branch);return true;
#else
 debug::silentInstallReason.store(11);
#endif
 return false;
}
'''
new=before+replacement+after
new=new.replace('#include "features/aim/debug.hpp"','#include "features/aim/debug.hpp"\n#include "features/aim/hook_support.hpp"',1)
assert new[new.index('inline void onPlayer(void* p){'):]==after
p.write_text(new)
for name in ['jni/src/game/offsets.hpp','jni/src/features/aim/debug.hpp','tests/aim/debug_smoke.cpp','scripts/debug/aim_debug.sh']:
 p=R/name;s=p.read_text();assert 'v10-aim-debug' in s;s=s.replace('v10-aim-debug','v11-aim-hook-fix').replace('No v10 heartbeat','No v11 heartbeat');p.write_text(s)
p=R/'jni/src/features/aim/debug.hpp';s=p.read_text();s=s.replace('case 11:return "unsupported-architecture";default:', 'case 11:return "unsupported-architecture";case 12:return "slot-verify-failed";case 13:return "profile-not-validated";default:');p.write_text(s)
p=R/'scripts/test.sh';s=p.read_text();anchor=' run_case debug_smoke tests/aim/debug_smoke.cpp -ldl -lpthread';assert s.count(anchor)==1;s=s.replace(anchor,anchor+'\n run_case hook_support tests/aim/hook_support_test.cpp -ldl -lpthread');p.write_text(s);p.chmod(0o755)
(R/'scripts/debug/aim_debug.sh').chmod(0o755)
print('Applied v11 installer fixes; target math, camera/ray handlers, world/local/attack/menu gates unchanged')
