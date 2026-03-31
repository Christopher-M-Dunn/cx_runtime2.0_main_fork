#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

void state_test() {
    int a = 3;
    int b = 5;
    int result;

    VLOG("Checking cx_selector_user CSR is 0 (legacy) initially...\n");
    uint cx_index = cx_csr_read(CX_SELECTOR_USER);
    assert ( cx_index == 0 );
    VLOG("  cx_selector_user = 0x%08x OK\n", cx_index);

    VLOG("Checking cx_error is 0 initially...\n");
    uint cx_error = cx_error_read();
    assert ( cx_error == 0 );
    VLOG("  cx_error = 0 OK\n");

    VLOG("Opening MULACC (CX_NO_VIRT)...\n");
    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_index = cx_csr_read(CX_SELECTOR_USER);
    assert ( cx_index == 0 );
    assert( cx_sel_A0 > 0 );
    VLOG("  cx_sel_A0 = 0x%08x, selector unchanged after open (0x%08x)\n", cx_sel_A0, cx_index);

    VLOG("Selecting cx_sel_A0...\n");
    cx_error_clear();
    cx_sel(cx_sel_A0);
    cx_index = cx_csr_read(CX_SELECTOR_USER);
    assert ( cx_index == cx_sel_A0 );
    VLOG("  cx_selector_user = 0x%08x\n", cx_index);

    uint status = CX_READ_STATUS();
    uint state_size = GET_CX_STATE_SIZE(status);
    VLOG("  state status = 0x%08x, state_size = %d\n", status, state_size);

    VLOG("Computing mac(%d, %d)...\n", a, b);
    result = mac(a, b);
    VLOG("  result = %d (expected 15)\n", result);
    assert( result == 15 );

    status = CX_READ_STATUS();
    state_size = GET_CX_STATE_SIZE(status);
    VLOG("  state status after mac = 0x%08x, state_size = %d\n", status, state_size);

    cx_error = cx_error_read();
    assert ( cx_error == 0 );
    VLOG("  cx_error = 0 OK\n");

    VLOG("Closing cx_sel_A0...\n");
    cx_close(cx_sel_A0);

    VLOG("Opening two more MULACC selectors (testing multiple physical states)...\n");
    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    int cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_A1 > 0 );
    assert( cx_sel_A2 > 0 );
    VLOG("  cx_sel_A1 = 0x%08x, cx_sel_A2 = 0x%08x\n", cx_sel_A1, cx_sel_A2);

    VLOG("Selecting A1, mac(%d, %d)...\n", a, a);
    cx_error_clear();
    cx_sel(cx_sel_A1);
    cx_index = cx_csr_read(CX_SELECTOR_USER);
    assert ( cx_index == cx_sel_A1 );
    result = mac(a, a);
    VLOG("  result = %d (expected 9)\n", result);
    assert( result == 9 );
    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    VLOG("Selecting A2, mac(%d, %d) (independent state)...\n", b, b);
    cx_error_clear();
    cx_sel(cx_sel_A2);
    cx_index = cx_csr_read(CX_SELECTOR_USER);
    assert ( cx_index == cx_sel_A2 );
    result = mac(b, b);
    VLOG("  result = %d (expected 25)\n", result);
    assert( result == 25 );
    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    VLOG("Closing A1...\n");
    cx_close(cx_sel_A1);

    VLOG("Reopening MULACC (state should be freed and reallocatable)...\n");
    uint cx_sel_test = -1;
    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    assert( cx_sel_test > 0 );
    VLOG("  cx_sel_test = 0x%08x\n", cx_sel_test);
    cx_close(cx_sel_test);

    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    assert( cx_sel_test > 0 );
    VLOG("  cx_sel_test reopened = 0x%08x\n", cx_sel_test);
    cx_close(cx_sel_test);

    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    cx_close(cx_sel_test);
    assert( cx_sel_test > 0 );

    cx_close(cx_sel_A2);

    VLOG("Testing invalid GUID (should return -1)...\n");
    const int INVALID_CX_GUID = -1;
    cx_sel_test = cx_open(INVALID_CX_GUID, CX_NO_VIRT, -1);
    VLOG("  cx_sel_test = %d (expected -1)\n", cx_sel_test);
    assert( cx_sel_test == -1 );

    VLOG("Selecting CX_LEGACY\n");
    cx_sel( CX_LEGACY );
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    cx_sel( CX_LEGACY );
    state_test();
    printf("state test passed\n");
    return 0;
}