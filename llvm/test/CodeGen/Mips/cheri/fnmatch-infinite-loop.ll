; RUN: %cheri_purecap_llc -o - -float-abi=soft -O0 -mips-ssection-threshold=0 %s | %cheri_FileCheck %s
; ModuleID = '/home/alr48/obj/build/llvm-build/fnmatch-ccd909-bugpoint-reduce.ll-reduced-simplified.bc'
source_filename = "fnmatch-ccd909-bugpoint-reduce.ll-output-0ec4103.bc"
target datalayout = "E-m:e-pf200:128:128:128:64-i8:8:32-i16:16:32-i64:64-n32:64-S128-A200"
target triple = "cheri-unknown-freebsd"

module asm ".ident\09\22$FreeBSD$\22"

%union.__mbstate_t.0.8.16.24.32.40.48.56.64 = type { i64, [120 x i8] }

@fnmatch.initial = external addrspace(200) constant %union.__mbstate_t.0.8.16.24.32.40.48.56.64, align 8

; Function Attrs: noinline nounwind optnone
declare i32 @fnmatch(ptr addrspace(200), ptr addrspace(200), i32 signext) #0

; Function Attrs: noinline nounwind optnone
define internal i32 @fnmatch1(ptr addrspace(200) %pattern, ptr addrspace(200) %string, ptr addrspace(200) %stringstart, i32 signext %flags, i64 inreg %patmbs.coerce0, i64 inreg %patmbs.coerce1, i64 inreg %patmbs.coerce2, i64 inreg %patmbs.coerce3, i64 inreg %patmbs.coerce4, i64 inreg %patmbs.coerce5, i64 inreg %patmbs.coerce6, i64 inreg %patmbs.coerce7, i64 inreg %patmbs.coerce8, i64 inreg %patmbs.coerce9, i64 inreg %patmbs.coerce10, i64 inreg %patmbs.coerce11, i64 inreg %patmbs.coerce12, i64 inreg %patmbs.coerce13, i64 inreg %patmbs.coerce14, i64 inreg %patmbs.coerce15, i64 inreg %strmbs.coerce0, i64 inreg %strmbs.coerce1, i64 inreg %strmbs.coerce2, i64 inreg %strmbs.coerce3, i64 inreg %strmbs.coerce4, i64 inreg %strmbs.coerce5, i64 inreg %strmbs.coerce6, i64 inreg %strmbs.coerce7, i64 inreg %strmbs.coerce8, i64 inreg %strmbs.coerce9, i64 inreg %strmbs.coerce10, i64 inreg %strmbs.coerce11, i64 inreg %strmbs.coerce12, i64 inreg %strmbs.coerce13, i64 inreg %strmbs.coerce14, i64 inreg %strmbs.coerce15) #0 {
entry:
  store ptr addrspace(200) null, ptr addrspace(200) undef, align 32
  unreachable
}
; CHECK-LABEL: fnmatch1:

; Function Attrs: nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.pcc.get() #1

; Function Attrs: nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.cap.offset.set.i64(ptr addrspace(200), i64) #1

declare i64 @mbrtowc(ptr addrspace(200), ptr addrspace(200), i64 zeroext, ptr addrspace(200)) #2

; Function Attrs: nounwind memory(read)
declare ptr addrspace(200) @strchr(ptr addrspace(200), i32 signext) #3

; Function Attrs: noinline nounwind optnone
declare i32 @rangematch(ptr addrspace(200), i32 signext, i32 signext, ptr addrspace(200), ptr addrspace(200)) #0

; Function Attrs: noinline nounwind optnone
declare i32 @__tolower(i32 signext) #0

declare ptr addrspace(200) @__get_locale() #2

declare i32 @__wcollate_range_cmp(i32 signext, i32 signext) #2

; Function Attrs: nounwind memory(read)
declare i32 @___tolower(i32 signext) #3

declare ptr addrspace(200) @__getCurrentRuneLocale() #2

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p200.i64(ptr addrspace(200) nocapture writeonly, i8, i64, i1 immarg) #4

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg) #5

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="true" }
attributes #1 = { nounwind willreturn memory(none) }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="true" }
attributes #3 = { nounwind memory(read) "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="true" }
attributes #4 = { nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #5 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

!llvm.ident = !{!0}

!0 = !{!"clang version 7.0.0 (https://github.com/llvm-mirror/clang.git 407982bdd68f598f7afdc16acb46deba1b59070a) (https://github.com/llvm-mirror/llvm.git 960f90bedf5ab44be8a48a74cdcec9cdd2eed62f)"}
