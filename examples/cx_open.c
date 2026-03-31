#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

int a = 5, b = 3, result = 0;

void exclusive_open() {
    VLOG("[exclusive_open] Opening MULACC (CX_NO_VIRT)...\n");
    cx_select_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(sel > 0);
    VLOG("  sel = 0x%08x\n", sel);
    VLOG("  Closing sel\n");
    cx_close(sel);
}

void inter_open() {
    VLOG("[inter_open] Opening MULACC (CX_INTER_VIRT)...\n");
    cx_select_t sel = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
    assert(sel > 0);
    VLOG("  sel = 0x%08x\n", sel);
    VLOG("  Closing sel\n");
    cx_close(sel);
}

void exclusive_open_2() {
    VLOG("[exclusive_open_2] Opening two MULACC selectors (CX_NO_VIRT)...\n");
    cx_select_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_select_t selB = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(selA > 0);
    assert(selB > 0);
    VLOG("  selA = 0x%08x, selB = 0x%08x\n", selA, selB);
    VLOG("  Closing selA, selB\n");
    cx_close(selA);
    cx_close(selB);
}

void exclusive_open_3() {
    VLOG("[exclusive_open_3] Opening 3 MULACC selectors (only 2 physical states; 3rd should fail)...\n");
    cx_select_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_select_t selB = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_select_t selC = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    printf("opens finished\n");
    VLOG("  selA = 0x%08x, selB = 0x%08x, selC = 0x%08x (expected <0)\n", selA, selB, selC);
    assert(selA > 0);
    assert(selB > 0);
    assert(selC < 0);

    VLOG("  Selecting selA, mac(%d, %d)...\n", a, a);
    cx_sel(selA);
    result = mac(a, a);
    VLOG("  result = %d (expected 25)\n", result);
    assert(result == 25);

    VLOG("  Selecting selB, mac(%d, %d)...\n", b, b);
    cx_sel(selB);
    result = mac(b, b);
    VLOG("  result = %d (expected 9)\n", result);
    assert(result == 9);

    VLOG("  Selecting selA, mac(%d, %d) (accumulates)...\n", a, b);
    cx_sel(selA);
    result = mac(a, b);
    VLOG("  result = %d (expected 40)\n", result);
    assert(result == 40);

    VLOG("  Selecting selB, mac(%d, %d) (accumulates)...\n", a, b);
    cx_sel(selB);
    result = mac(a, b);
    VLOG("  result = %d (expected 24)\n", result);
    assert(result == 24);

    VLOG("  Closing selA, selB\n");
    cx_close(selA);
    cx_close(selB);
}

void exclusive_open_4() {
    VLOG("[exclusive_open_4] Opening two MULACC selectors, using selA...\n");
    cx_select_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_select_t selB = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(selA > 0);
    assert(selB > 0);
    VLOG("  selA = 0x%08x, selB = 0x%08x\n", selA, selB);
    VLOG("  Selecting selA, mac(%d, %d)...\n", b, b);
    cx_sel( selA );
    result = mac(b, b);
    VLOG("  result = %d (expected 9)\n", result);
    assert(result == 9);
    VLOG("  Closing selA, selB, selecting CX_LEGACY\n");
    cx_close(selA);
    cx_close(selB);
    cx_sel(CX_LEGACY);
}

void mixed_open_1() {
    VLOG("[mixed_open_1] Open, use, close, reopen MULACC...\n");
    cx_select_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(sel > 0);
    VLOG("  sel = 0x%08x\n", sel);
    VLOG("  Selecting sel, mac(%d, %d)...\n", a, a);
    cx_sel(sel);
    result = mac(a, a);
    VLOG("  result = %d (expected 25)\n", result);
    assert(result == 25);
    VLOG("  Closing sel, reopening...\n");
    cx_close(sel);
    sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(sel > 0);
    VLOG("  new sel = 0x%08x, closing\n", sel);
    cx_close(sel);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    // exclusive_open();
    // inter_open();
    // exclusive_open_2();
    exclusive_open_3();
    // exclusive_open_4();
    // mixed_open_1();
    printf("cx_open test passed!\n");
    return 0;
}