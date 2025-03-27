#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/ci_m.h"
#include "../include/list.h"

#include "../zoo/max/max_common.h"
#include "../zoo/mulacc/mulacc_common.h"
#include "../zoo/nn_acc/nn_acc_common.h"
#include "../zoo/vector/vector_common.h"

#define CX_AVAIL_STATE 1
#define CX_UNAVAIL_STATE 0

typedef struct cx_state_data_t {
    int status;
    int v_id;
    uint *data;
    struct list_head list;
} cx_state_data_t;

typedef struct cx_state_info_t {
    CX_VIRT_T virt;
    // when the counter is 0, we can set the CX_VIRT_T. Until it becomes
    // 0'ed again, we must respect that all newly opened virtual
    // contexts are of the same virt type, or else the cx_open will fail.
    struct list_head vstate;
    uint vstate_id[4]; // 128 virtual IDs per state context.
    int vcounter;
    int prev_used_vid; // To know if context save + restore is needed
} cx_state_info_t;

typedef struct {
  // static
  int cx_guid;
  int num_states;

  // dynamic
  int *avail_state_ids;
  cx_state_info_t *state_info;
} cx_entry_m_t;

typedef cx_entry_m_t cx_map_t;
cx_map_t cx_map[NUM_CXUS];

static inline cx_select_t gen_cx_sel(cxu_id_t cxu_id, cx_state_id_t state_id, 
                                     cx_vstate_id_t vstate_id) 
{
    cx_idx_t cx_sel = {.sel = {   .cxu_id = cxu_id, 
                                  .state_id = state_id,
                                  .v_state_id = vstate_id,
                                  .cxe = 0,
                                  .version = 1}};
    return cx_sel.idx;
}

static int get_free_state(cxu_id_t cxu_id) {
    int lowest_use_state = -1;
    int lowest_use_state_count = 0x7FFFFFFF;
    for (int i = 0; i < cx_map[cxu_id].num_states; i++) {
        if (cx_map[cxu_id].avail_state_ids[i] == 1) {
            cx_map[cxu_id].avail_state_ids[i] = 0;
            return i;
        }
        if (lowest_use_state_count > cx_map[cxu_id].state_info[i].vcounter) {
            lowest_use_state = i;
            lowest_use_state_count = cx_map[cxu_id].state_info[i].vcounter;
        }
    }

    // Only room to virtualize 128 times
    if (lowest_use_state_count >= 128) {
        return -1;
    }

    return lowest_use_state;
}

static int get_free_vstate(cxu_id_t cxu_id, cx_state_id_t state_id) {
    for (int i = 0; i < 4; i++) { // 4 * 32 = 128, number of virt_id's per physical state context
        if (cx_map[cxu_id].state_info[state_id].vstate_id[i] != 0xFFFFFFFF) {
            cx_vstate_id_t vstate = cx_map[cxu_id].state_info[state_id].vstate_id[i];
            for (int j = 0; j < 32; j++) { // iterating over bits
                if (GET_BITS(vstate, j, 1) == 0) {
                    vstate |= (1 << j);
                    cx_map[cxu_id].state_info[state_id].vstate_id[i] = vstate;
                    cx_map[cxu_id].state_info[state_id].vcounter++;
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

static void free_vstate(cxu_id_t cxu_id, cx_state_id_t state_id, cx_vstate_id_t vstate_id) {
    cx_map[cxu_id].state_info[state_id].vstate_id[vstate_id / 4] |= (1 << (vstate_id % 32));
    cx_map[cxu_id].state_info[state_id].vcounter--;
}

static bool is_state_unused(cxu_id_t cxu_id, cx_state_id_t state_id) {
    for (int i = 0; i < 4; i++) {
        if (cx_map[cxu_id].state_info[state_id].vstate_id[i] != 0) {
            return false;
        }
    }
    return true;
}

static int is_valid_state_id(cxu_id_t cxu_id, cx_state_id_t state_id) 
{
    if (state_id < 0) {
        return false; // No available states for cx_guid 
    } else if (state_id > cx_map[cxu_id].num_states - 1) {
        return false;
    }
    return true;
}

// populates the cx_map
void cx_init() {
    // zero the mcx_selector
    cx_csr_write(MCX_SELECTOR, 0);
    
    // 0 initialize the cx_status csr
    // cx_csr_write(CX_STATUS, 0);

    cx_map[0].cx_guid = CX_GUID_VECTOR;
    cx_map[1].cx_guid = CX_GUID_MULACC;
    cx_map[2].cx_guid = CX_GUID_MAX;
    cx_map[3].cx_guid = CX_GUID_NN_ACC;

    cx_map[0].num_states = CX_VECTOR_NUM_STATES;
    cx_map[1].num_states = CX_MULACC_NUM_STATES;
    cx_map[2].num_states = CX_MAX_NUM_STATES;
    cx_map[3].num_states = CX_NN_ACC_NUM_STATES;

    for (int i = 0; i < NUM_CXUS; i++) {
        if (cx_map[i].num_states) {
            cx_map[i].avail_state_ids = malloc(cx_map[i].num_states * sizeof(int));
            cx_map[i].state_info = malloc(cx_map[i].num_states * sizeof(cx_state_info_t));
            for (int j = 0; j < cx_map[i].num_states; j++) {
                INIT_LIST_HEAD(&cx_map[i].state_info[j].vstate);
                cx_map[i].avail_state_ids[j] = 1;
                cx_map[i].state_info[j].virt = -1;
                cx_map[i].state_info[j].vcounter = 0;
                cx_map[i].state_info[j].prev_used_vid = -1;
                for (int k = 0; k < 4; k++) {
                    cx_map[i].state_info[j].vstate_id[k] = 0;
                }
            }
        } else {
            cx_map[i].state_info = malloc(1 * sizeof(cx_state_info_t));
            cx_map[i].state_info[0].prev_used_vid = -1;
        }
    }
}

void context_save(cx_state_data_t *data) {
    cxu_sctx_t status = CX_READ_STATUS();
    int size = GET_CX_STATE_SIZE(status);
    for (int i = 0; i < size; i++) {
        data->data[i] = CX_READ_STATE(i);
    }

    cx_stctxs_t status_word = { .idx = status };
    status_word.sel.cs = CX_CLEAN;
    data->status = status_word.idx;
}

void context_restore(cx_state_data_t *data) {
    int size = GET_CX_STATE_SIZE(data->status);
    for (int i = 0; i < size; i++) {
        CX_WRITE_STATE(i, data->data[i]);
    }

    CX_WRITE_STATUS(data->status);
}

void context_switch(cxu_id_t cxu_id, cx_state_id_t state_id, cx_vstate_id_t vstate_id, cx_vstate_id_t prev_vstate_id) {

    // context switching needed
    
    cx_state_data_t *new_state_data = NULL, *prev_state_data = NULL, *tmp_state_data = NULL;
    list_for_each_entry(tmp_state_data, &cx_map[cxu_id].state_info[state_id].vstate, list) {
        if (tmp_state_data->v_id == prev_vstate_id) {
            prev_state_data = tmp_state_data;
        }

        if (tmp_state_data->v_id == vstate_id) {
            new_state_data = tmp_state_data;
        }

        if (new_state_data && prev_state_data) {
            break;
        }
    }

    context_save(prev_state_data);
    context_restore(new_state_data);
}

// void cx_sel(cx_select_t cx_sel) {
//     // cx_idx_t new_sel =  { .idx = cx_sel };
//     // cxu_id_t cxu_id = new_sel.sel.cxu_id;
//     // cx_state_id_t state_id = new_sel.sel.state_id;
//     // cx_vstate_id_t vstate_id = new_sel.sel.v_state_id;

//     // int prev_used_vid = cx_map[cxu_id].state_info[state_id].prev_used_vid;
//     cx_csr_write(MCX_SELECTOR, cx_sel);
    
//     // if (prev_used_vid != -1 &&
//     //     vstate_id != prev_used_vid) {
//     //     printf("Context Switching\n");
//     //     context_switch(cxu_id, state_id, vstate_id, prev_used_vid);
//     // }
//     // cx_map[cxu_id].state_info[state_id].prev_used_vid = vstate_id;
// }

static void initialize_state() {

    uint status = CX_READ_STATUS();
    uint sw_init = GET_CX_INITIALIZER(status);

    // 4. Read the state to get the state_size
    cx_stctxs_t stat = { .idx = status };
    uint state_size = stat.sel.state_size;
    // printf("state_size: %d\n", state_size);

    // 5. Set the CXU to initial state
    stat.sel.cs = CX_INITIAL;
    CX_WRITE_STATUS(stat.idx);

    // With R=0, there could still be software initialization that needs
    // to be done.
    if (stat.sel.cs == CX_INITIAL) {
        for (int i = 0; i < state_size; i++) {
            CX_WRITE_STATE(i, 0);
        }
    }
    stat.sel.cs = CX_DIRTY;
    CX_WRITE_STATUS(stat.idx);
}

static int alloc_sel(cxu_id_t cxu_id) {
    cx_state_id_t state_id = get_free_state(cxu_id);
        
    if (!is_valid_state_id(cxu_id, state_id)) {
        return -1;
    }

    // cx_vstate_id_t vstate_id = get_free_vstate(cxu_id, state_id);
    cx_vstate_id_t vstate_id = 0;

    // if (vstate_id == -1) {
    //     return -1;
    // }

    cx_select_t new_cx_sel = gen_cx_sel(cxu_id, state_id, vstate_id);
    // cx_state_data_t *state = malloc(sizeof(cx_state_data_t));
    // if (!state) {
    //     return -1;
    // }
    
    // state->data = malloc(sizeof(uint) * 1);
    // if (!state->data) {
    //     return -1;
    // }

    // state->v_id = vstate_id;
    // list_add(&state->list, &cx_map[cxu_id].state_info[state_id].vstate);
    return new_cx_sel;
}


int32_t cx_open(cx_guid_t cx_guid, cx_virt_t cx_virt, cx_select_t user_cx_sel) {
    static int initialized = 0;
    
    if (!initialized) {
        cx_init();
        initialized = 1;
        // printf("Initializing!\n");
    }

    cxu_id_t cxu_id = -1;
    for (int j = 0; j < NUM_CXUS; j++) {
        if (cx_map[j].cx_guid == cx_guid) {
            cxu_id = j;
            break;
        }
    }
    
    if (cxu_id == -1) {
       return -1;
    }

    if (cx_map[cxu_id].num_states == 0) {
        return gen_cx_sel(cxu_id, 0, 0);
    } else {

        cx_select_t new_cx_sel = alloc_sel(cxu_id);
        if (new_cx_sel == -1) {
            return -1;
        }

        cx_select_t prev_sel = cx_csr_read(MCX_SELECTOR);
        cx_sel(new_cx_sel);

        initialize_state();

        cx_sel(prev_sel);

        return new_cx_sel;
    }
}


void cx_close(cx_select_t cx_sel)
{
  cxu_id_t cxu_id = CX_GET_CXU_ID(cx_sel);
  
  if (cxu_id >= NUM_CXUS) {
    return;
  }
  // Stateless cx's
  if (cx_map[cxu_id].num_states == 0) {
    return;
  } else { // Stateful
    cx_state_id_t state_id = CX_GET_STATE_ID(cx_sel);
    // cx_vstate_id_t vstate_id = CX_GET_VIRT_STATE_ID(cx_sel);
    cx_map[cxu_id].avail_state_ids[state_id] = 1;
    
    // cx_state_data_t* state_data;
//     list_for_each_entry(state_data, &cx_map[cxu_id].state_info[state_id].vstate, list) {
//         if (state_data->v_id == vstate_id) {
//             list_del(&state_data->list);
//             free(state_data->data);
//             break;
//         }
//     }

//     free_vstate(cxu_id, state_id, vstate_id);

//     if (is_state_unused(cxu_id, state_id)) {
//         cx_map[cxu_id].state_info[state_id].virt = -1;
//     }
//     if (vstate_id == cx_map[cxu_id].state_info[state_id].prev_used_vid)
//         cx_map[cxu_id].state_info[state_id].prev_used_vid = -1;
  }
}

cx_error_t cx_error_read() {
  return cx_csr_read(CX_STATUS);
}

void cx_error_clear() {
  cx_csr_write(CX_STATUS, 0);
}
