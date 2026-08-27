#include "sub_0000_tensors.h"

const TensorInfo sub_0000_tensors[] = {
  { "_split_1_command_stream", 1, 2204, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 2, 3264, "MODEL", 0xffffffff },
  { "_split_1_scratch", 3, 224, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 4, 224, "FAST_SCRATCH", 0x0 },
  { "serving_default_x_0", 5, 39, "INPUT_TENSOR", 0x0 },
  { "StatefulPartitionedCall_0_70009", 0, 4, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0000_tensors_count = sizeof(sub_0000_tensors) / sizeof(sub_0000_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0000_address_serving_default_x_0 = 0x0;
const uint32_t sub_0000_address_StatefulPartitionedCall_0_70009 = 0x0;

