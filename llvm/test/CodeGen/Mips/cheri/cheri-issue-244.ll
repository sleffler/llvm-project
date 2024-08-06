; RUN: %cheri_purecap_llc %s -o /dev/null

%struct.b = type { ptr addrspace(200) }

@e = local_unnamed_addr addrspace(200) global %struct.b { ptr addrspace(200) @c }, align 32

; Function Attrs: nounwind
define internal i32 @c() addrspace(200) #0 {
entry:
  %call = tail call addrspace(200) i32 (...) @dladdr(ptr addrspace(200) inreg @c) #0
  ret i32 undef
}

; Function Attrs: nounwind
declare i32 @dladdr(...) addrspace(200) #0

attributes #0 = { nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 1}
!2 = !{!"clang version 5.0.0 "}
