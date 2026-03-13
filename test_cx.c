#include <stdio.h>
#include "include/ci.h"
#include "include/utils.h"

int main() {
    int val1 = 100;
    int val2 = 50;
    int result;

    printf("Starting CXU AddSub Test...\n");

    // 1. Open the CX interface (GUID 0 for addsub usually)
    cx_open(0, 0, 0);

    // 2. Select the first accelerator (ID 0 = addsub)
    cx_sel(0);

    // 3. Execute! 
    // In zoo/addsub, cf_id 0 is typically 'Add'
    result = CX_REG_HELPER(0, val1, val2);
    printf("Add Test: %d + %d = %d\n", val1, val2, result);

    // In zoo/addsub, cf_id 1 is typically 'Sub'
    result = CX_REG_HELPER(1, val1, val2);
    printf("Sub Test: %d - %d = %d\n", val1, val2, result);

    return 0;
}
