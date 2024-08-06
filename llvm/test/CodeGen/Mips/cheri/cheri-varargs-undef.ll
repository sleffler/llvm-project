; RUN: %cheri_purecap_llc -O0 %s -o - | FileCheck %s --check-prefixes=UNDEF,ALIGNED-ARG,ALIGNED-VARARG
; ModuleID = 'unions.c'
; XFAIL: *
; This test is horribly fragile and it doesn't document what it's supposed to
; be checking, so no one except Khilan can fix it.
source_filename = "unions.c"

%union.meh = type { ptr addrspace(200) }

@undef.meh = private unnamed_addr addrspace(200) constant { i32, [12 x i8] } { i32 42, [12 x i8] undef }, align 16
@aligned_arg.meh = private unnamed_addr addrspace(200) constant { i32, [12 x i8] } { i32 42, [12 x i8] undef }, align 16
@aligned_vararg.meh = private unnamed_addr addrspace(200) constant { i32, [12 x i8] } { i32 42, [12 x i8] undef }, align 16

; Function Attrs: noinline nounwind optnone
define void @a(i32 signext %cmd, ...) addrspace(200) #0 {
entry:
  %cmd.addr = alloca i32, align 4, addrspace(200)
  %ap = alloca ptr addrspace(200), align 16, addrspace(200)
  %meh = alloca %union.meh, align 16, addrspace(200)
  store i32 %cmd, ptr addrspace(200) %cmd.addr, align 4
  %ap1 = bitcast ptr addrspace(200) %ap to ptr addrspace(200)
  call void @llvm.va_start.p200(ptr addrspace(200) %ap1)
  %argp.cur = load ptr addrspace(200), ptr addrspace(200) %ap, align 16
  %0 = call i64 @llvm.cheri.cap.offset.get.i64(ptr addrspace(200) %argp.cur)
  %1 = add i64 %0, 15
  %2 = and i64 %1, -16
  %3 = call ptr addrspace(200) @llvm.cheri.cap.offset.set.i64(ptr addrspace(200) %argp.cur, i64 %2)
  %argp.next = getelementptr inbounds i8, ptr addrspace(200) %3, i64 16
  store ptr addrspace(200) %argp.next, ptr addrspace(200) %ap, align 16
  %4 = bitcast ptr addrspace(200) %3 to ptr addrspace(200)
  %5 = bitcast ptr addrspace(200) %meh to ptr addrspace(200)
  %6 = bitcast ptr addrspace(200) %4 to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %5, ptr addrspace(200) align 16 %6, i64 16, i1 false)
  call void @llvm.va_end.p200(ptr addrspace(200) %ap1)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p200(ptr addrspace(200)) addrspace(200) #1

; Function Attrs: nounwind willreturn memory(none)
declare i64 @llvm.cheri.cap.offset.get.i64(ptr addrspace(200)) addrspace(200) #2

; Function Attrs: nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.cap.offset.set.i64(ptr addrspace(200), i64) addrspace(200) #2

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p200(ptr addrspace(200)) addrspace(200) #1

; Function Attrs: noinline nounwind optnone
define void @b(i32 signext %cmd, i32 signext %x, ...) addrspace(200) #0 {
entry:
  %cmd.addr = alloca i32, align 4, addrspace(200)
  %x.addr = alloca i32, align 4, addrspace(200)
  %ap = alloca ptr addrspace(200), align 16, addrspace(200)
  %meh = alloca %union.meh, align 16, addrspace(200)
  store i32 %cmd, ptr addrspace(200) %cmd.addr, align 4
  store i32 %x, ptr addrspace(200) %x.addr, align 4
  %ap1 = bitcast ptr addrspace(200) %ap to ptr addrspace(200)
  call void @llvm.va_start.p200(ptr addrspace(200) %ap1)
  %argp.cur = load ptr addrspace(200), ptr addrspace(200) %ap, align 16
  %0 = call i64 @llvm.cheri.cap.offset.get.i64(ptr addrspace(200) %argp.cur)
  %1 = add i64 %0, 15
  %2 = and i64 %1, -16
  %3 = call ptr addrspace(200) @llvm.cheri.cap.offset.set.i64(ptr addrspace(200) %argp.cur, i64 %2)
  %argp.next = getelementptr inbounds i8, ptr addrspace(200) %3, i64 16
  store ptr addrspace(200) %argp.next, ptr addrspace(200) %ap, align 16
  %4 = bitcast ptr addrspace(200) %3 to ptr addrspace(200)
  %5 = bitcast ptr addrspace(200) %meh to ptr addrspace(200)
  %6 = bitcast ptr addrspace(200) %4 to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %5, ptr addrspace(200) align 16 %6, i64 16, i1 false)
  call void @llvm.va_end.p200(ptr addrspace(200) %ap1)
  ret void
}

; Function Attrs: noinline nounwind optnone
define void @c(i32 signext %cmd, ...) addrspace(200) #0 {
entry:
  %cmd.addr = alloca i32, align 4, addrspace(200)
  %ap = alloca ptr addrspace(200), align 16, addrspace(200)
  %meh = alloca %union.meh, align 16, addrspace(200)
  %j = alloca i32, align 4, addrspace(200)
  %vaarg.promotion-temp = alloca i32, align 4, addrspace(200)
  store i32 %cmd, ptr addrspace(200) %cmd.addr, align 4
  %ap1 = bitcast ptr addrspace(200) %ap to ptr addrspace(200)
  call void @llvm.va_start.p200(ptr addrspace(200) %ap1)
  %argp.cur = load ptr addrspace(200), ptr addrspace(200) %ap, align 16
  %argp.next = getelementptr inbounds i8, ptr addrspace(200) %argp.cur, i64 8
  store ptr addrspace(200) %argp.next, ptr addrspace(200) %ap, align 16
  %0 = bitcast ptr addrspace(200) %argp.cur to ptr addrspace(200)
  %1 = load i64, ptr addrspace(200) %0, align 8
  %2 = trunc i64 %1 to i32
  store i32 %2, ptr addrspace(200) %vaarg.promotion-temp, align 4
  %3 = load i32, ptr addrspace(200) %vaarg.promotion-temp, align 4
  store i32 %3, ptr addrspace(200) %j, align 4
  %argp.cur2 = load ptr addrspace(200), ptr addrspace(200) %ap, align 16
  %4 = call i64 @llvm.cheri.cap.offset.get.i64(ptr addrspace(200) %argp.cur2)
  %5 = add i64 %4, 15
  %6 = and i64 %5, -16
  %7 = call ptr addrspace(200) @llvm.cheri.cap.offset.set.i64(ptr addrspace(200) %argp.cur2, i64 %6)
  %argp.next3 = getelementptr inbounds i8, ptr addrspace(200) %7, i64 16
  store ptr addrspace(200) %argp.next3, ptr addrspace(200) %ap, align 16
  %8 = bitcast ptr addrspace(200) %7 to ptr addrspace(200)
  %9 = bitcast ptr addrspace(200) %meh to ptr addrspace(200)
  %10 = bitcast ptr addrspace(200) %8 to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %9, ptr addrspace(200) align 16 %10, i64 16, i1 false)
  call void @llvm.va_end.p200(ptr addrspace(200) %ap1)
  ret void
}

; Function Attrs: noinline nounwind optnone
define void @undef() addrspace(200) #0 {
entry:
  %meh = alloca %union.meh, align 16, addrspace(200)
  %0 = bitcast ptr addrspace(200) %meh to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %0, ptr addrspace(200) align 16 @undef.meh, i64 16, i1 false)
  %coerce.dive = getelementptr inbounds %union.meh, ptr addrspace(200) %meh, i32 0, i32 0
  %1 = load ptr addrspace(200), ptr addrspace(200) %coerce.dive, align 16
  ; UNDEF-LABEL: undef:
  ; UNDEF: cincoffset $c24, $c11, $zero
  ; UNDEF: cincoffset [[TMPSP:\$c[0-9]+]], $c11, {{128|80}}
  ; UNDEF-NEXT: csetbounds	{{.*}}, [[TMPSP]], {{32|16}}
  call void (i32, ...) @a(i32 signext 99, i64 undef, ptr addrspace(200) inreg %1)
  ret void
}

; Function Attrs: nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.pcc.get() addrspace(200) #2

; Function Attrs: noinline nounwind optnone
define void @aligned_arg() addrspace(200) #0 {
entry:
  %meh = alloca %union.meh, align 16, addrspace(200)
  %0 = bitcast ptr addrspace(200) %meh to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %0, ptr addrspace(200) align 16 @aligned_arg.meh, i64 16, i1 false)
  %coerce.dive = getelementptr inbounds %union.meh, ptr addrspace(200) %meh, i32 0, i32 0
  %1 = load ptr addrspace(200), ptr addrspace(200) %coerce.dive, align 16
  ; ALIGNED-ARG-LABEL: aligned_arg:
  ; ALIGNED-ARG: cincoffset	$c[[TMP:[0-9]+]], $c11, {{128|80}}
  ; ALIGNED-ARG-NEXT:csetbounds	$c{{[0-9]+}}, $c[[TMP]], {{32|16}}
  call void (i32, i32, ...) @b(i32 signext 13, i32 signext 37, ptr addrspace(200) inreg %1)
  ret void
}

; Function Attrs: noinline nounwind optnone
define void @aligned_vararg() addrspace(200) #0 {
entry:
  %meh = alloca %union.meh, align 16, addrspace(200)
  %0 = bitcast ptr addrspace(200) %meh to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %0, ptr addrspace(200) align 16 @aligned_vararg.meh, i64 16, i1 false)
  %coerce.dive = getelementptr inbounds %union.meh, ptr addrspace(200) %meh, i32 0, i32 0
  %1 = load ptr addrspace(200), ptr addrspace(200) %coerce.dive, align 16
  ; ALIGNED-VARARG-LABEL: aligned_vararg:
  ; ALIGNED-VARARG: cincoffset	{{.*}}, $c11, 12
  call void (i32, ...) @c(i32 signext 99, i32 signext 37, ptr addrspace(200) inreg %1)
  ret void
}

; Function Attrs: noinline nounwind optnone
define i32 @main() addrspace(200) #0 {
entry:
  %retval = alloca i32, align 4, addrspace(200)
  store i32 0, ptr addrspace(200) %retval, align 4
  call void @undef()
  call void @aligned_arg()
  call void @aligned_vararg()
  ret i32 0
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg) addrspace(200) #3

attributes #0 = { noinline nounwind optnone }
attributes #1 = { nocallback nofree nosync nounwind willreturn }
attributes #2 = { nounwind willreturn memory(none) }
attributes #3 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{!"clang version 5.0.0 (git@github.com:CTSRD-CHERI/clang.git f592f4eab1e8d9994f233fcf13c984caa3b4b913) (https://github.com/CTSRD-CHERI/llvm.git 3f41fd54c7337547f976f82705594c3e2287f346)"}
