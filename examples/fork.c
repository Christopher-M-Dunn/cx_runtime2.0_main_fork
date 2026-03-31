#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/addsub/addsub.h"
#include "../zoo/muldiv/muldiv.h"

#include <sys/types.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <unistd.h>

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

static const int a = 3, b = 5, c = 2;
static int result = 0;
static cx_stctxs_t expected_stctxs = {.sel = {
                                .dc = CX_DIRTY,
                                .state_size = 1
                              }};

/*
* Ensuring that the fork function works on whichever device
* or emulator this is run on
*/
void test_fork() {
  VLOG("[test_fork] Basic fork sanity check...\n");
  pid_t p = fork();
  assert(p >= 0);
  if (p == 0) {
    exit(EXIT_SUCCESS);
  } else {
    wait(NULL);
  }
  VLOG("  OK\n");
  return;
}

/*
* Open and closing in a single process
*/
void test_fork_0() {
  VLOG("[test_fork_0] Child opens/uses/closes CX, parent waits...\n");
  int result;

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  pid_t pid = fork();
  assert(pid >= 0);

  if (pid == 0) {
    VLOG("  [child] Opening MULACC (CX_NO_VIRT)...\n");
    int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(cx_sel_C0 != -1);
    VLOG("  [child] cx_sel_C0 = 0x%08x, selecting + mac(%d,%d)...\n", cx_sel_C0, b, b);
    cx_error_clear();
    cx_sel(cx_sel_C0);
    result = mac(b, b);
    VLOG("  [child] result = %d (expected 25)\n", result);
    assert( result == 25 );
    cx_close(cx_sel_C0);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Waiting for child...\n");
    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }
  return;
}


/*
* Open and closing in both processes
*/
void test_fork_1() {
  VLOG("[test_fork_1] Both parent and child open/use/close CX...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  pid_t pid = fork();
  assert(pid >= 0);

  if (pid == 0) {
    VLOG("  [child] Opening MULACC (CX_NO_VIRT), mac(%d,%d)...\n", b, b);
    int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(cx_sel_C0 != -1);
    cx_error_clear();
    cx_sel(cx_sel_C0);
    result = mac(b, b);
    VLOG("  [child] result = %d (expected 25)\n", result);
    assert( result == 25 );
    cx_close(cx_sel_C0);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Opening MULACC (CX_NO_VIRT), mac(%d,%d)...\n", c, c);
    int cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(cx_sel_C1 != -1);
    cx_error_clear();
    cx_sel(cx_sel_C1);
    result = mac(c, c);
    VLOG("  [parent] result = %d (expected 4)\n", result);
    assert( result == 4 );
    cx_close(cx_sel_C1);

    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }
  return;
}

/*
* Strictly stateless open and close
*/
void test_fork_2() {
  VLOG("[test_fork_2] Child uses stateless ADDSUB...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  pid_t pid = fork();

  if (pid < 0){
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    VLOG("  [child] Opening ADDSUB (CX_NO_VIRT)...\n");
    int cx_sel_C2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C2 != -1 );
    VLOG("  [child] cx_sel_C2 = 0x%08x\n", cx_sel_C2);

    cx_sel(cx_sel_C2);
    result = add(a, b);
    VLOG("  [child] add(%d,%d) = %d (expected 8)\n", a, b, result);
    assert( result == 8 );
    result = add(a, c);
    VLOG("  [child] add(%d,%d) = %d (expected 5)\n", a, c, result);
    assert( result == 5 );

    cx_close(cx_sel_C2);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Waiting for child...\n");
    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }
}


/*
* Open and closing in both processes, with some more complexities.
*/
void test_fork_3() {
  VLOG("[test_fork_3] Both parent and child use multiple CXs...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  pid_t pid = fork();

  if (pid < 0){
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    VLOG("  [child] Opening MULACC + ADDSUB...\n");
    cx_select_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C1 != -1 );
    assert( cx_sel_C2 != -1 );

    cx_sel(cx_sel_C1);
    result = mac(b, c); VLOG("  [child] mac(%d,%d)=%d (exp 10)\n",b,c,result); assert( result == 10 );
    result = mac(b, c); VLOG("  [child] mac(%d,%d)=%d (exp 20)\n",b,c,result); assert( result == 20 );
    result = mac(b, c); VLOG("  [child] mac(%d,%d)=%d (exp 30)\n",b,c,result); assert( result == 30 );

    cx_sel(cx_sel_C2);
    result = add(a, b); VLOG("  [child] add(%d,%d)=%d (exp 8)\n",a,b,result);  assert( result == 8 );
    result = add(a, c); VLOG("  [child] add(%d,%d)=%d (exp 5)\n",a,c,result);  assert( result == 5 );

    cx_sel(cx_sel_C1);
    result = mac(a, c); VLOG("  [child] mac(%d,%d)=%d (exp 36)\n",a,c,result); assert( result == 36 );

    cx_close(cx_sel_C1);
    cx_close(cx_sel_C2);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Opening MULDIV + ADDSUB...\n");
    cx_select_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 != -1 );
    assert( cx_sel_C5 != -1 );

    cx_sel(cx_sel_C4);
    result = mul(a, b); VLOG("  [parent] mul(%d,%d)=%d (exp 15)\n",a,b,result); assert( result == 15 );
    result = mul(c, b); VLOG("  [parent] mul(%d,%d)=%d (exp 10)\n",c,b,result); assert( result == 10 );
    result = mul(a, c); VLOG("  [parent] mul(%d,%d)=%d (exp 6)\n",a,c,result);  assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b); VLOG("  [parent] add(%d,%d)=%d (exp 8)\n",a,b,result);  assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }
}

void complex_fork_test() {
  VLOG("[complex_fork_test] Parent opens INTER_VIRT before fork, both use after...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
  assert(cx_sel_C0 != -1);
  VLOG("  cx_sel_C0 = 0x%08x\n", cx_sel_C0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  mac(%d,%d)=%d (exp 25)\n", b, b, result);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(CX_SELECTOR_USER);

  cx_error = cx_error_read();
  assert( cx_error == 0 );

  cx_status = CX_READ_STATUS();

  VLOG("  Forking...\n");
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    VLOG("  [child] Opening MULACC + ADDSUB...\n");
    cx_select_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C1 != -1 );
    assert( cx_sel_C2 != -1 );

    cx_sel(cx_sel_C1);
    result = mac(b, c); VLOG("  [child] mac=%d (exp 10)\n",result); assert( result == 10 );
    result = mac(b, c); VLOG("  [child] mac=%d (exp 20)\n",result); assert( result == 20 );
    result = mac(b, c); VLOG("  [child] mac=%d (exp 30)\n",result); assert( result == 30 );

    cx_sel(cx_sel_C2);
    result = add(a, b); VLOG("  [child] add=%d (exp 8)\n",result);  assert( result == 8 );
    result = add(a, c); VLOG("  [child] add=%d (exp 5)\n",result);  assert( result == 5 );

    cx_sel(cx_sel_C1);
    result = mac(a, c); VLOG("  [child] mac=%d (exp 36)\n",result); assert( result == 36 );

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C1);
    cx_close(cx_sel_C2);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Opening MULDIV + ADDSUB...\n");
    cx_select_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 != -1 );
    assert( cx_sel_C5 != -1 );

    cx_sel(cx_sel_C4);
    result = mul(a, b); VLOG("  [parent] mul=%d (exp 15)\n",result); assert( result == 15 );
    result = mul(c, b); VLOG("  [parent] mul=%d (exp 10)\n",result); assert( result == 10 );
    result = mul(a, c); VLOG("  [parent] mul=%d (exp 6)\n",result);  assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b); VLOG("  [parent] add=%d (exp 8)\n",result);  assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);

    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  [parent] post-fork mac=%d\n", result);
  cx_close(cx_sel_C0);
  cx_sel(CX_LEGACY);
}

void use_prev_opened_in_child() {
  VLOG("[use_prev_opened_in_child] Child uses parent's pre-fork selector...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
  assert(cx_sel_C0 != -1);
  VLOG("  cx_sel_C0 = 0x%08x\n", cx_sel_C0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  pre-fork mac(%d,%d)=%d (exp 25)\n", b, b, result);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(CX_SELECTOR_USER);

  cx_error = cx_error_read();
  assert( cx_error == 0 );

  cx_status = CX_READ_STATUS();

  VLOG("  Forking...\n");
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    VLOG("  [child] Opening MULACC, then using parent's cx_sel_C0...\n");
    cx_select_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_C1 != -1 );

    cx_sel(cx_sel_C0);
    result = mac(b, c); VLOG("  [child] mac=%d (exp 35)\n",result); assert( result == 35 );
    result = mac(b, c); VLOG("  [child] mac=%d (exp 45)\n",result); assert( result == 45 );
    result = mac(b, c); VLOG("  [child] mac=%d (exp 55)\n",result); assert( result == 55 );

    cx_sel(cx_sel_C1);
    result = mac(a, c); VLOG("  [child] mac=%d (exp 6)\n",result); assert( result == 6 );

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C1);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Opening MULDIV + ADDSUB...\n");
    cx_select_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 != -1 );
    assert( cx_sel_C5 != -1 );

    cx_sel(cx_sel_C4);
    result = mul(a, b); VLOG("  [parent] mul=%d (exp 15)\n",result); assert( result == 15 );
    result = mul(c, b); VLOG("  [parent] mul=%d (exp 10)\n",result); assert( result == 10 );
    result = mul(a, c); VLOG("  [parent] mul=%d (exp 6)\n",result);  assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b); VLOG("  [parent] add=%d (exp 8)\n",result);  assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);

    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }

  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  [parent] post-fork mac=%d (exp 50)\n", result);
  assert (result == 50 );

  cx_close(cx_sel_C0);
  cx_sel(CX_LEGACY);
}

void use_prev_opened_in_parent() {
  VLOG("[use_prev_opened_in_parent] Parent uses pre-fork selector after fork...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
  assert(cx_sel_C0 != -1);
  VLOG("  cx_sel_C0 = 0x%08x\n", cx_sel_C0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  pre-fork mac=%d (exp 25)\n", result);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(CX_SELECTOR_USER);

  cx_error = cx_error_read();
  assert( cx_error == 0 );

  cx_status = CX_READ_STATUS();

  VLOG("  Forking...\n");
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    VLOG("  [child] Opening MULDIV + ADDSUB...\n");
    cx_select_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 != -1 );
    assert( cx_sel_C5 != -1 );

    cx_sel(cx_sel_C4);
    result = mul(a, b); VLOG("  [child] mul=%d (exp 15)\n",result); assert( result == 15 );
    result = mul(c, b); VLOG("  [child] mul=%d (exp 10)\n",result); assert( result == 10 );
    result = mul(a, c); VLOG("  [child] mul=%d (exp 6)\n",result);  assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b); VLOG("  [child] add=%d (exp 8)\n",result);  assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    cx_close(cx_sel_C0);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Using pre-fork cx_sel_C0 + new MULACC...\n");
    cx_select_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_C1 != -1 );

    cx_sel(cx_sel_C0);
    result = mac(b, c); VLOG("  [parent] mac=%d (exp 35)\n",result); assert( result == 35 );
    result = mac(b, c); VLOG("  [parent] mac=%d (exp 45)\n",result); assert( result == 45 );
    result = mac(b, c); VLOG("  [parent] mac=%d (exp 55)\n",result); assert( result == 55 );

    cx_sel(cx_sel_C1);
    result = mac(a, c); VLOG("  [parent] mac=%d (exp 6)\n",result); assert( result == 6 );

    cx_close(cx_sel_C1);
    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }

  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  [parent] post-fork mac=%d (exp 80)\n", result);
  assert ( result == 80 );

  cx_close(cx_sel_C0);
  cx_sel(CX_LEGACY);
}

void use_prev_opened_in_parent_and_child() {
  VLOG("[use_prev_opened_in_parent_and_child] Both use pre-fork selector concurrently...\n");
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
  assert(cx_sel_C0 != -1);
  VLOG("  cx_sel_C0 = 0x%08x\n", cx_sel_C0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  pre-fork mac=%d (exp 25)\n", result);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(CX_SELECTOR_USER);

  cx_error = cx_error_read();
  assert( cx_error == 0 );

  cx_status = CX_READ_STATUS();

  VLOG("  Forking...\n");
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    VLOG("  [child] Using cx_sel_C0 + MULDIV + ADDSUB...\n");
    cx_select_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_select_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 != -1 );
    assert( cx_sel_C5 != -1 );

    cx_sel(cx_sel_C0);
    result = mac(b, b); VLOG("  [child] mac=%d (exp 50)\n",result);  assert(result == 50);
    result = mac(b, b); VLOG("  [child] mac=%d (exp 75)\n",result);  assert(result == 75);
    result = mac(b, b); VLOG("  [child] mac=%d (exp 100)\n",result); assert(result == 100);
    result = mac(b, b); VLOG("  [child] mac=%d (exp 125)\n",result); assert(result == 125);

    cx_sel(cx_sel_C4);
    result = mul(a, b); VLOG("  [child] mul=%d (exp 15)\n",result); assert( result == 15 );
    result = mul(c, b); VLOG("  [child] mul=%d (exp 10)\n",result); assert( result == 10 );
    result = mul(a, c); VLOG("  [child] mul=%d (exp 6)\n",result);  assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b); VLOG("  [child] add=%d (exp 8)\n",result);  assert(result == 8);

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    exit(EXIT_SUCCESS);
  } else {
    VLOG("  [parent] Using cx_sel_C0 + new MULACC...\n");
    cx_select_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_C1 != -1 );

    cx_sel(cx_sel_C0);
    result = mac(b, c); VLOG("  [parent] mac=%d (exp 35)\n",result); assert( result == 35 );
    result = mac(b, c); VLOG("  [parent] mac=%d (exp 45)\n",result); assert( result == 45 );
    result = mac(b, c); VLOG("  [parent] mac=%d (exp 55)\n",result); assert( result == 55 );

    cx_sel(cx_sel_C1);
    result = mac(a, c); VLOG("  [parent] mac=%d (exp 6)\n",result); assert( result == 6 );

    cx_close(cx_sel_C1);

    int status;
    waitpid(pid, &status, 0);
    assert(status == 0);
    VLOG("  [parent] Child exited OK\n");
  }

  cx_sel(cx_sel_C0);
  result = mac(b, b);
  VLOG("  [parent] post-fork mac=%d (exp 80)\n", result);
  assert (result == 80 );

  cx_close(cx_sel_C0);
  cx_sel(CX_LEGACY);
}


void close_unclosed_cx() {

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_select_t cx_select, cx_select_0;

  cx_select_t cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
  assert(cx_sel_C0 != -1);

  cx_select_t cx_sel_C1;

  int *glob_counter;
  glob_counter = mmap(NULL, sizeof *glob_counter, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *glob_counter = 0;

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    cx_select_t cx_sel_C4 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    if (cx_sel_C4 != -1)
      *glob_counter += 1;

    exit(EXIT_SUCCESS);
  } else {
    cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    if (cx_sel_C1 != -1)
      *glob_counter += 1;

    int status;
    wait(NULL);
    assert(status == 0);
    printf("counter: %d\n", *glob_counter);
    assert(*glob_counter == 1);
    munmap(glob_counter, sizeof *glob_counter);
  }
  cx_select_t cx_sel_C5 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
  assert( cx_sel_C5 < 0 );

  cx_close(cx_sel_C0);
  cx_close(cx_sel_C5);

  cx_select_t cx_sel_C6 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
  cx_select_t cx_sel_C7 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);

  assert(cx_sel_C6 != -1);
  assert(cx_sel_C7 != -1);

  cx_close(cx_sel_C6);
  cx_close(cx_sel_C7);

  cx_sel(CX_LEGACY);
}


int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }

    cx_sel(CX_LEGACY);
    VLOG("=== test_fork ===\n");
    test_fork();
    VLOG("=== test_fork_0 ===\n");
    test_fork_0();
    VLOG("=== test_fork_1 ===\n");
    test_fork_1();
    VLOG("=== test_fork_2 ===\n");
    test_fork_2();
    VLOG("=== test_fork_3 ===\n");
    test_fork_3();
    VLOG("=== complex_fork_test ===\n");
    complex_fork_test();
    VLOG("=== use_prev_opened_in_child ===\n");
    use_prev_opened_in_child();
    VLOG("=== use_prev_opened_in_parent ===\n");
    use_prev_opened_in_parent();
    VLOG("=== use_prev_opened_in_parent_and_child ===\n");
    use_prev_opened_in_parent_and_child();
    //   close_unclosed_cx();

    printf("Fork Test Complete\n");
    return 0;
}