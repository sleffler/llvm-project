// RUN: %cheri_purecap_cc1 %s -emit-obj -o - -fcxx-exceptions -fexceptions -debug-info-kind=standalone \
// RUN:   | llvm-dwarfdump -debug-frame - | FileCheck %s -check-prefixes CHECK
// Also try assembling with integrated as and verify that the return and stack registers are correct
// RUN: %cheri_purecap_cc1 %s -S -o %t.s -fcxx-exceptions -fexceptions -debug-info-kind=standalone
// RUN: %cheri_purecap_clang -c %t.s -o - | llvm-dwarfdump -debug-frame - | FileCheck %s -check-prefixes CHECK
// RUN: %cheri_purecap_llvm-mc %t.s -o - -filetype=obj | llvm-dwarfdump -debug-frame - | FileCheck %s -check-prefixes CHECK
int test(int arg1, int arg2) {
  return arg1 + arg2;
}

// CHECK: .debug_frame contents:
// CHECK-EMPTY:
// CHECK-NEXT: 00000000 000000{{.+}} ffffffff CIE
// CHECK-NEXT:  Format: DWARF32
// CHECK-NEXT: Version:               4
// CHECK-NEXT:  Augmentation:          ""
// CHECK-NEXT: Address size:          8
// CHECK-NEXT: Segment desc size:     0
// CHECK-NEXT:  Code alignment factor: 1
// CHECK-NEXT:  Data alignment factor: -8
// CHECK-NEXT:  Return address column: 89
// CHECK-EMPTY:
// CHECK-NEXT: DW_CFA_def_cfa_register: C11
