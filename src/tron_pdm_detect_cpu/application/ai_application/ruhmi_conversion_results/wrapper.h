#ifndef MODEL_WRAPPER_H
#define MODEL_WRAPPER_H

#include "model.h"
#include "model_io_data.h"
#include <stdint.h>
#include <stdbool.h>

static inline int8_t* mera_input_ptr() {
    return GetModelInputPtr_serving_default_x_0();
}

static inline int8_t* mera_output_ptr() {
    return GetModelOutputPtr_StatefulPartitionedCall_0_70021();
}

static inline uint64_t mera_input_size() {
    return 650;
}

static inline uint64_t mera_output_size() {
    return 5;
}

static inline void mera_invoke() {
    RunModel(false);
}

#endif // MODEL_WRAPPER_H
