#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/ci.h"
#include "../zoo/muldiv/muldiv.h"
#include "../zoo/addsub/addsub.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

void test() {
    int a = 3, b = 5, result = 0;

    VLOG("Opening MULDIV (CX_NO_VIRT)...\n");
    cx_select_t selM = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    assert(selM > 0);
    VLOG("  selM = 0x%08x\n", selM);

    VLOG("Opening ADDSUB (CX_NO_VIRT)...\n");
    cx_select_t selA = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert(selA > 0);
    assert(selA != selM);
    VLOG("  selA = 0x%08x\n", selA);

    VLOG("Opening ADDSUB again (stateless: should return same selector)...\n");
    cx_select_t selA0 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert(selA0 > 0);
    assert(selA0 == selA);
    VLOG("  selA0 = 0x%08x (== selA)\n", selA0);

    VLOG("Selecting ADDSUB, computing add(%d, %d)...\n", a, b);
    cx_sel(selA);
    VLOG("  active selector: 0x%08x\n", cx_csr_read(CX_SELECTOR_USER));
    result = add(a, b);
    VLOG("  result = %d (expected 8)\n", result);
    printf("selA: %08x, selM: %08x, res: %d\n", selA, selM, result);
    assert(result == 8);

    VLOG("Selecting MULDIV, computing mul(%d, %d)...\n", a, b);
    cx_sel(selM);
    result = mul(a, b);
    VLOG("  result = %d (expected 15)\n", result);
    assert(result == 15);

    VLOG("Selecting ADDSUB, computing add(%d, %d)...\n", a, a);
    cx_sel(selA);
    result = add(a, a);
    VLOG("  result = %d (expected 6)\n", result);
    assert(result == 6);

    VLOG("Selecting ADDSUB via selA0, computing add(%d, %d)...\n", b, b);
    cx_sel(selA0);
    result = add(b, b);
    VLOG("  result = %d (expected 10)\n", result);
    assert(result == 10);

    VLOG("Closing selA0, selA, selM...\n");
    cx_close(selA0);
    cx_close(selA);
    cx_close(selM);

    VLOG("Selecting CX_LEGACY\n");
    cx_sel( CX_LEGACY );
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    test();
    printf("stateless test passed!\n");
    return 0;
}