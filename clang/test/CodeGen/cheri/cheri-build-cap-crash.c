// UNSUPPORTED: true
// RUN: %cheri_purecap_cc1 %s -o - -emit-llvm -O1 -Wunsafe-buffer-usage -verify | FileCheck %s
// TODO - This is crashing when -Wunsafe-buffer-usage is used
// Likely introduced with this commit: a046d1877201
// expected-no-diagnostics

#include <cheriintrin.h>

void test(void *__capability cap, char *__capability cap2);
void test(void *__capability cap, char *__capability cap2) {
// CHECK-LABEL: @test(
// CHECK-NEXT:  entry:
    cheri_cap_build(cap, (__intcap_t)cap2);
}
