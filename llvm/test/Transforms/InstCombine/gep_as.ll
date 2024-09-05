; RUN: opt -passes=instcombine -S < %s | FileCheck %s
target datalayout = "pf200:128:128:128:64"
target triple = "riscv64-unknown-freebsd"

define i32 @main() {
entry:
  ; Check that the address space cast happens before the GEP.
  ; CHECK: %0 = addrspacecast ptr %call to ptr addrspace(200)
  ; CHECK: %add.ptr = getelementptr inbounds i32, ptr addrspace(200) %0, i64 41
  %call = call ptr @malloc(i64 zeroext 168)
  %0 = bitcast ptr %call to ptr
  %1 = addrspacecast ptr %0 to ptr addrspace(200)
  call void @test(ptr addrspace(200) %1)
  %add.ptr = getelementptr inbounds i32, ptr addrspace(200) %1, i64 41
  call void @test(ptr addrspace(200) %add.ptr)
  ret i32 0
}

declare ptr @malloc(i64 zeroext)

declare void @test(ptr addrspace(200))
