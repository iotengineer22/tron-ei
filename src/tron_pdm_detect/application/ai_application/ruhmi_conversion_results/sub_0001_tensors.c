#include "sub_0001_tensors.h"

const TensorInfo sub_0001_tensors[] = {
  { "_split_1_command_stream", 1, 2496, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 2, 3680, "MODEL", 0xffffffff },
  { "_split_1_scratch", 3, 1056, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 4, 1056, "FAST_SCRATCH", 0x0 },
  { "sequential_conv1d_Conv1D_ExpandDims1_70011", 5, 650, "INPUT_TENSOR", 0x0 },
  { "StatefulPartitionedCall_0_70021", 0, 5, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0001_tensors_count = sizeof(sub_0001_tensors) / sizeof(sub_0001_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0001_address_sequential_conv1d_Conv1D_ExpandDims1_70011 = 0x0;
const uint32_t sub_0001_address_StatefulPartitionedCall_0_70021 = 0x0;

