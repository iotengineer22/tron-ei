/*
 * This file is developed by EdgeCortix Inc. to be used with certain Renesas Electronics Hardware only.
 *
 * Copyright © 2025 EdgeCortix Inc. Licensed to Renesas Electronics Corporation with the
 * right to sublicense under the Apache License, Version 2.0.
 *
 * This file also includes source code originally developed by the Renesas Electronics Corporation.
 * The Renesas disclaimer below applies to any Renesas-originated portions for usage of the code.
 *
 * The Renesas Electronics Corporation
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED 'AS IS' AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Changed from original python code to C source code.
 * Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
 *
 * This file also includes source codes originally developed by the TensorFlow Authors which were distributed under the following conditions.
 *
 * The TensorFlow Authors
 * Copyright 2023 The Apache Software Foundation
 *
 * This product includes software developed at
 * The Apache Software Foundation (http://www.apache.org/).
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#include "compute_sub_0000.h"

#include "arm_nn_types.h"
#include "arm_nnfunctions.h"
#include "kernel_library_utils.h"

#include "kernel_library_int.h" 

 

void compute_sub_0000(
  // buffer for intermediate results
  uint8_t* main_storage, // should provide at least 769 bytes of storage

  // inputs
  
  const int8_t serving_default_x_0[650], // 1,650
  

  // outputs
  
  int8_t StatefulPartitionedCall_0_70021[5]  // 1,5
  
) {
  // Buffers allocated on the main storage (note: depends on the execution order)
    
  
  int8_t* sequential_conv1d_1_Relu_sequential_conv1d_1_BiasAdd_sequential_conv1d_1_Conv1D_Squeeze_sequential_conv1d_1_BiasAdd_ReadVariableOp_sequential_conv1d_1_Conv1D_70016 = (int8_t *) &main_storage[0]; // 1,1,25,16 == 400
  
  int8_t* sequential_conv1d_Relu_sequential_conv1d_BiasAdd_sequential_conv1d_Conv1D_Squeeze__sequential_conv1d_Conv1D_70012 = (int8_t *) &main_storage[0]; // 1,1,50,8 == 400
  
  int8_t* sequential_max_pooling1d_1_MaxPool_70018 = (int8_t *) &main_storage[400]; // 1,13,1,16 == 208
  
  int8_t* sequential_max_pooling1d_MaxPool_70014 = (int8_t *) &main_storage[400]; // 1,25,1,8 == 200
  
  int8_t* sequential_y_pred_MatMul_sequential_y_pred_BiasAdd_70020 = (int8_t *) &main_storage[0]; // 1,5 == 5
  
  

  // Parameters
  
  
  static const int32_t Int32VecConstant_70005[5] = { // 5
    -481, -67, 940, -361, 567, 
  };
  
  static const int32_t Int32VecConstant_70007[16] = { // 16
    -183, -760, -370, -429, -4827, -196, 344, -3103, -689, 454, 260, -899, -573, -4961, -96, -761, 
  };
  
  static const int32_t Int32VecConstant_70009[8] = { // 8
    -113, -308, -6635, -2330, -13791, -1780, -5997, -5254, 
  };
  
  static const int8_t Int8VecConstant_70006[1040] = { // 5,208
    3, -26, -7, 4, -19, 1, 0, -16, 15, 1, 9, 0, 5, 13, -2, -12, -1, -20, -9, 1, -2, -3, 4, -8, 11, -11, -1, -4, 5, 8, -1, -18, -5, -18, -7, 0, -1, -3, 1, -4, 13, 4, 13, -11, 4, 8, 7, -22, -7, -20, -15, -6, -7, 1, 7, -8, 15, 15, 10, 3, 8, 6, -4, -23, -9, -29, -10, -5, -9, -3, 7, 2, 12, 15, 8, 3, 1, 7, -10, -27, -5, -16, -11, -3, -6, -1, 7, -2, 21, 20, 13, -4, 1, 11, -12, -18, -5, -14, -8, -5, -8, -7, 10, -10, 9, 18, 12, 0, 0, 7, -8, -18, -13, -9, -8, 1, -10, -2, 12, -3, 14, 23, 10, 2, -1, 8, -14, -13, -8, -12, -10, -2, -8, 5, 10, -11, 14, 12, 11, -1, 2, 7, -12, -8, -8, -11, -3, -2, -11, -3, 13, -5, 12, 18, 12, 0, -2, 9, -11, -3, -8, -5, -13, -3, 4, -7, 14, -2, 10, 24, 11, -3, 2, 10, -10, -7, -8, -3, -11, -9, -3, -3, 12, -2, 10, 17, 12, 2, 4, 9, -15, -15, 3, 8, -2, -1, -11, 2, 2, 7, 4, 8, 5, -9, 4, -1, -9, -15, 4, 12, -7, 8, -15, 6, 3, -1, 11, 5, -7, 0, 6, 11, -5, 10, -5, 8, -8, -17, -10, 5, -8, -2, -13, -2, 2, -5, -2, 7, -13, 9, 13, 8, -7, -10, -2, 1, -3, 4, -2, -1, -11, -5, 2, 9, 3, 7, 10, 14, -9, -7, -11, 10, -11, 2, 2, -2, -12, -2, 0, 13, 5, 5, 15, 12, -5, -6, -4, 3, -5, 0, 3, -11, -16, -8, 1, 8, 13, 13, 14, 10, -7, -10, -15, 3, -8, 1, 0, -12, -12, -9, 7, 10, 11, 10, 15, 14, -7, -6, -9, 8, -8, -1, -5, -11, -15, -8, 3, 9, 13, 14, 17, 11, -11, -2, -8, 4, -5, 2, 1, -20, -10, -8, 3, 8, 15, 6, 13, 10, -9, -13, -8, 3, -9, 3, 0, -17, -11, -3, 2, 7, 7, 8, 18, 10, -9, -7, -12, 5, -11, 4, -9, -6, -16, -4, -4, 9, 9, 5, 15, 6, -4, -8, -10, 4, -12, 1, -3, -16, -16, -3, -2, 6, 9, 4, 4, 11, -6, -3, -9, 5, -7, -1, -3, -7, -9, 3, -2, 3, 6, 1, 4, -2, 9, -4, 14, 7, -7, -1, -4, -1, -12, -13, -3, -1, 4, 2, -42, -47, -2, -10, -11, 16, -19, 4, -23, 6, -25, -11, -42, -66, -44, -39, -3, -26, 10, -2, -25, 14, -5, -26, -22, -11, -23, -34, -44, -20, -22, -27, -14, -69, 17, -30, -57, 10, -36, -36, -20, 0, -25, -10, -58, -61, -14, -38, -45, -70, 12, -14, -1, 18, -30, -46, -14, -15, -18, -40, -55, -84, -7, -59, -9, -75, 15, -6, -61, 15, -21, -29, -25, -8, -23, -33, -47, -96, -5, -32, -38, -100, 13, -43, -29, 20, -21, -64, -23, -3, -37, -19, -24, -112, -22, -68, -45, -66, 12, -25, -25, 11, -25, -55, -30, 0, -46, -27, -21, -83, -15, -30, -56, -59, 0, -23, -40, 23, -18, -50, -33, -4, -49, -39, -42, -49, -18, -55, -62, -49, 27, -12, -47, 18, -21, -32, -34, -5, -41, -27, -42, -50, -25, -49, -26, -79, 14, -31, -21, 14, -19, -40, -22, -14, -29, -38, -21, -73, -4, -49, -12, -127, 17, -23, -7, 14, -23, -39, -37, -17, -29, -30, -27, -86, -12, -38, 0, -43, -1, -22, -22, 10, -16, -22, -34, -16, -34, -67, -29, -66, -16, -45, -75, -115, -15, -36, 8, 19, -37, -67, -3, 9, -12, -61, -38, -21, 0, -37, 4, 9, -1, -34, -8, -3, -6, -12, -11, -12, 2, -10, 9, -16, 0, 10, 2, 6, 7, -2, -8, 4, -12, 1, -9, 9, 0, 16, 5, -4, 3, 8, -29, 10, 9, 8, -2, -5, -1, -9, -9, -12, -8, 16, 4, -6, 4, 7, -21, 11, 7, 19, -7, 1, -6, 0, -3, -9, -11, 19, 5, -8, 4, 11, -17, 7, 6, 14, -4, -3, -13, -3, -2, -5, -16, 14, 4, -5, -3, 11, -16, 7, 2, 16, 1, -8, -6, -6, 0, -18, -13, 12, 9, -4, 2, 10, -10, 7, 4, 16, -4, -8, -4, -1, -7, -17, -11, 20, 7, -8, -2, 12, -12, 5, 10, 19, 3, 2, -2, -1, -8, -7, -12, 12, 7, -15, 1, 14, -15, 6, 0, 13, 1, -8, -4, -3, -2, -9, -14, 18, 9, 3, 6, 10, -7, 8, 11, 12, -6, -7, 2, -3, -13, -10, -18, 10, 1, -12, 2, 13, -23, 8, 5, 7, -4, -4, -8, -10, -5, -20, -13, 5, 7, -5, 0, 7, -17, 8, 0, 8, 1, -1, -2, 3, -8, -12, -17, 12, 3, -6, 6, 11, -7, -1, 5, 8, -15, -6, -7, -6, -6, -15, -16, 22, 6, -14, 2, -32, 1, -10, -1, 3, 7, -8, 9, 1, -11, 1, -2, -2, -12, -6, 3, -12, 5, -14, 2, -6, 13, -15, 3, 15, -2, -5, 4, -12, -9, -17, 3, -15, 3, -15, 6, -5, 15, -14, 15, 14, -3, 4, 10, -15, -5, -5, 4, -21, 4, -13, 6, -11, 9, -14, 11, 13, -11, -15, 9, -7, -7, -6, -1, -18, 4, -16, 6, -7, 19, -8, 12, 16, -12, -8, 6, -9, -11, -8, -5, -12, 8, -13, 5, -13, 18, -12, 4, 17, -8, -4, 13, -18, -6, -18, -5, -11, 5, -8, 10, -4, 17, -5, 4, 15, -8, -3, 11, -6, -9, -11, -3, -13, 3, -7, 9, -7, 9, -12, 10, 17, -12, -3, 9, -11, -6, -12, -5, -13, 7, -8, 10, -3, 12, -6, 4, 15, -1, -3, 5, -6, -1, -12, -7, -8, 3, -14, 4, -12, 13, -3, 12, 16, -2, -10, 6, -14, -8, -12, -1, -6, 12, -16, 9, -14, 12, -2, 7, 0, -6, -6, 7, -9, -3, -5, -1, -19, 2, -11, -1, -5, 3, -5, 10, -1, -14, -3, 2, -9, -7, -15, -1, 2, 6, 8, -7, 3, -27, -9, 3, 2, -17, -4, 6, -15, -10, 4, -1, 33, 
  };
  
  static const int8_t Int8VecConstant_70008[384] = { // 16,1,3,8
    -29, -14, 17, -1, -3, 1, 0, -5, -2, -26, -10, 1, -15, 0, -3, -20, 5, -10, -17, 1, -53, 1, -7, -127, 1, 1, -21, -3, -2, -65, -4, 2, -15, -1, -10, -31, -1, -17, 22, 0, -127, -12, 1, -4, 5, -1, -1, 4, 3, 6, -86, -8, -127, -14, 15, -32, -9, 11, -27, -23, -74, -4, -5, -75, -35, 0, 20, -10, -27, 31, 18, -39, -38, -127, 8, 13, -6, -1, -13, 3, -28, -32, -6, 0, 6, -9, -36, 8, -8, -5, -13, -8, 1, -44, -39, 7, 26, 5, -89, -2, -69, 0, -39, -76, -6, 14, -44, -18, -41, -12, -11, -50, -27, 5, 127, -33, -11, -2, -14, -18, 0, 17, -127, -2, -66, 7, -79, -43, -8, -3, -30, -10, -1, 10, -2, -24, -2, 10, -18, -10, -2, -34, -4, 27, 1, -23, 20, -11, -11, -30, 43, 0, -1, -11, -8, 0, -32, -105, -22, 13, 1, -8, -127, 16, -22, -123, -15, 7, 4, 17, -23, -21, -9, -7, -4, 0, -127, 17, 59, -72, 0, -8, -2, 4, -12, -19, -7, -47, 4, -6, 8, 0, -28, -19, -4, 7, 32, 9, -27, 2, -39, -56, 0, -1, -12, 5, -17, -33, -48, -81, -1, -18, -27, 18, 19, -127, -127, -35, 1, -94, -63, -3, 3, -5, -101, -80, 3, -26, -45, -33, -1, -3, -27, -119, -2, -7, -23, -82, -8, 15, -85, -26, 16, -63, -3, 14, 1, -9, -41, -23, -1, -127, -21, -12, 2, -6, -17, -14, 0, -95, -22, -22, 9, 1, -65, -73, 2, 3, -10, 5, 7, -18, -33, -127, 1, 3, -5, 1, -6, -11, -36, -73, 4, 12, 1, -1, -25, 25, 15, 1, 0, -8, 27, -18, -59, -127, -1, 2, -31, -6, 26, -30, -11, 6, 11, 2, -35, -18, -42, -29, 18, 0, 28, 24, -74, -9, 1, -68, 13, 54, -10, -5, -22, -2, -31, -36, -10, 0, -77, -47, -4, -9, 127, 6, -10, -29, 2, -98, 12, 0, 3, 2, -10, -4, -1, -9, -11, 7, -5, 1, -42, -7, 3, -22, -23, 4, -22, -3, -127, -11, -7, 8, -127, -31, -53, -114, -2, 6, -7, 2, -28, -48, 14, -42, -18, 3, -45, -9, -11, -12, 5, -14, 57, -2, 
  };
  
  static const int8_t Int8VecConstant_70010[312] = { // 8,1,3,13
    -45, -45, 34, -29, -22, 23, 18, -22, -1, -2, 2, 8, -16, -14, -80, 0, 9, 7, 5, 2, -5, 26, 20, -11, -10, 2, -38, -127, 33, 0, 12, -20, 6, 14, 9, 8, 29, 0, 6, -127, -34, -11, 13, -1, -6, -8, 6, 4, 2, 4, 12, -12, 9, 5, -17, -25, 4, -1, 9, 7, -16, 14, 10, -9, 6, 55, 18, 13, 9, -11, 9, -2, 0, -4, -13, 2, 1, -2, 109, 5, -69, 8, -13, 41, 22, 4, -4, 1, -24, -21, 10, 127, 38, -43, -14, -22, -5, 6, 11, 9, 15, 1, 4, -15, 48, 25, 42, 15, -20, -17, -3, 5, -25, 10, 9, -13, -3, 37, 39, -82, -52, 59, 9, -29, 22, 37, 0, -33, -40, 19, 64, -49, 6, 21, 23, -38, -44, 39, 42, -7, -17, -6, -18, -23, -127, 25, 96, -8, -95, 11, 16, 18, -5, 7, 16, -3, 126, -37, 86, 114, -127, -55, -92, 2, -14, 24, 17, 39, 39, 126, 17, 11, 88, -95, -109, -72, -4, 52, 29, -1, -21, -38, 122, 30, -120, 41, -15, 1, -33, 32, 45, 22, -27, -63, 23, -56, 17, -44, -6, 20, -15, -7, 19, 25, 0, -7, -8, 8, 127, -28, 7, -4, -2, -3, -10, 15, 0, 9, -10, -12, -13, 32, -24, -28, -16, 2, 27, 26, -13, 10, 2, 2, 1, -17, -15, 71, 18, -25, -38, 25, 11, -47, -11, 4, 9, 20, 16, -1, 61, 37, -24, -27, -1, -26, -44, -24, 11, -8, 17, -3, 82, 127, -42, -61, -23, 25, -4, -29, 7, -5, 27, 14, -7, -3, 32, -2, 75, 3, -90, -33, 19, 6, -31, 16, 32, 5, 37, 72, 80, 106, -48, -50, -64, -40, -46, 3, 39, 39, -8, -9, 76, 27, 127, -49, -52, -45, -40, -46, -16, 37, 16, -8, 
  };
  
  



// Declare the CMSIS Buffer with computed buffer size
int8_t* cmsis_buf = (int8_t *) &main_storage[608];
cmsis_nn_context ctx = {
  .buf = cmsis_buf,
#if defined(ARM_MATH_MVEI)
  .size =  192
#elif defined(ARM_MATH_DSP)
  .size =  160
#else
  .size =  160
#endif
};





//
// Identity - bypassing sequential_conv1d_Conv1D_ExpandDims1_70011 operation
//
// Input serving_default_x_0: int8_t - 1,650
// Output sequential_conv1d_Conv1D_ExpandDims1_70011: int8_t - 1,1,50,13


const int8_t* sequential_conv1d_Conv1D_ExpandDims1_70011 = serving_default_x_0;





//
// CMSIS-NN qconv2d
//
{
cmsis_nn_conv_params conv_params = {
  .dilation.h = 1,
  .dilation.w = 1,
  .input_offset = -1,
  .output_offset = -128,
  .stride.h = 1,
  .stride.w = 1,
  .padding.h = 0,
  .padding.w = 1,
  .activation.min = -128,
  .activation.max = 127
};

int32_t per_channel_multiplier[8] = { 1134846102, 1285237945, 2109136787, 2085170185, 1109272865, 1818605499, 1657652721, 1232098151,  };

int32_t per_channel_shift[8] = { -6, -5, -7, -7, -7, -6, -7, -7,  };

cmsis_nn_per_channel_quant_params quant_params = {
  .multiplier = per_channel_multiplier,
  .shift = per_channel_shift
};

cmsis_nn_dims input_dims = {
  .n = 1,
  .h = 1,
  .w = 50,
  .c = 13
};

cmsis_nn_dims filter_dims = {
  .n = 8,
  .h = 1,
  .w = 3,
  .c = 13
};

cmsis_nn_dims bias_dims = {
  .n = 1,
  .h = 1,
  .w = 1,
  .c = 8
};

cmsis_nn_dims output_dims = {
  .n = 1,
  .h = 1,
  .w = 50,
  .c = 8
};

arm_convolve_wrapper_s8(&ctx, &conv_params, &quant_params, &input_dims,
  sequential_conv1d_Conv1D_ExpandDims1_70011, &filter_dims, Int8VecConstant_70010, &bias_dims, Int32VecConstant_70009, &output_dims, sequential_conv1d_Relu_sequential_conv1d_BiasAdd_sequential_conv1d_Conv1D_Squeeze__sequential_conv1d_Conv1D_70012);
}


//
// Identity - bypassing sequential_max_pooling1d_ExpandDims_70013 operation
//
// Input sequential_conv1d_Relu_sequential_conv1d_BiasAdd_sequential_conv1d_Conv1D_Squeeze__sequential_conv1d_Conv1D_70012: int8_t - 1,1,50,8
// Output sequential_max_pooling1d_ExpandDims_70013: int8_t - 1,50,1,8


 int8_t* sequential_max_pooling1d_ExpandDims_70013 = sequential_conv1d_Relu_sequential_conv1d_BiasAdd_sequential_conv1d_Conv1D_Squeeze__sequential_conv1d_Conv1D_70012;





//
// CMSIS-NN qMaxPool2d
//
{
cmsis_nn_pool_params pool_params = {
  .stride.h = 2,
  .stride.w = 1,
  .padding.h = 0,
  .padding.w = 0,
  .activation.min = -128,
  .activation.max = 127
};

cmsis_nn_dims input_dims = {
  .n = 1,
  .h = 50,
  .w = 1,
  .c = 8
};

cmsis_nn_dims filter_dims = {
  .n = 1,
  .h = 2,
  .w = 1,
  .c = 1
};

cmsis_nn_dims output_dims = {
  .n = 1,
  .h = 25,
  .w = 1,
  .c = 8
};

cmsis_nn_context ctx = {
  .buf = 0,
  .size = 0 // "size" is not currently used my CMSIS-NN
};

arm_max_pool_s8(
  &ctx,
  &pool_params,
  &input_dims,
  sequential_max_pooling1d_ExpandDims_70013,
  &filter_dims,
  &output_dims,
  sequential_max_pooling1d_MaxPool_70014
);
}


//
// Identity - bypassing sequential_conv1d_1_Conv1D_ExpandDims_70015 operation
//
// Input sequential_max_pooling1d_MaxPool_70014: int8_t - 1,25,1,8
// Output sequential_conv1d_1_Conv1D_ExpandDims_70015: int8_t - 1,1,25,8


 int8_t* sequential_conv1d_1_Conv1D_ExpandDims_70015 = sequential_max_pooling1d_MaxPool_70014;





//
// CMSIS-NN qconv2d
//
{
cmsis_nn_conv_params conv_params = {
  .dilation.h = 1,
  .dilation.w = 1,
  .input_offset = 128,
  .output_offset = -128,
  .stride.h = 1,
  .stride.w = 1,
  .padding.h = 0,
  .padding.w = 1,
  .activation.min = -128,
  .activation.max = 127
};

int32_t per_channel_multiplier[16] = { 1491565170, 1188771342, 1974249080, 1446100581, 1245920428, 1243239471, 1146338128, 2134814958, 1561105092, 2144701422, 1334906177, 1312369808, 1439201842, 1960164785, 1456161490, 1974825465,  };

int32_t per_channel_shift[16] = { -4, -4, -6, -4, -6, -5, -5, -6, -5, -5, -5, -4, -5, -7, -4, -6,  };

cmsis_nn_per_channel_quant_params quant_params = {
  .multiplier = per_channel_multiplier,
  .shift = per_channel_shift
};

cmsis_nn_dims input_dims = {
  .n = 1,
  .h = 1,
  .w = 25,
  .c = 8
};

cmsis_nn_dims filter_dims = {
  .n = 16,
  .h = 1,
  .w = 3,
  .c = 8
};

cmsis_nn_dims bias_dims = {
  .n = 1,
  .h = 1,
  .w = 1,
  .c = 16
};

cmsis_nn_dims output_dims = {
  .n = 1,
  .h = 1,
  .w = 25,
  .c = 16
};

arm_convolve_wrapper_s8(&ctx, &conv_params, &quant_params, &input_dims,
  sequential_conv1d_1_Conv1D_ExpandDims_70015, &filter_dims, Int8VecConstant_70008, &bias_dims, Int32VecConstant_70007, &output_dims, sequential_conv1d_1_Relu_sequential_conv1d_1_BiasAdd_sequential_conv1d_1_Conv1D_Squeeze_sequential_conv1d_1_BiasAdd_ReadVariableOp_sequential_conv1d_1_Conv1D_70016);
}


//
// Identity - bypassing sequential_max_pooling1d_1_ExpandDims_70017 operation
//
// Input sequential_conv1d_1_Relu_sequential_conv1d_1_BiasAdd_sequential_conv1d_1_Conv1D_Squeeze_sequential_conv1d_1_BiasAdd_ReadVariableOp_sequential_conv1d_1_Conv1D_70016: int8_t - 1,1,25,16
// Output sequential_max_pooling1d_1_ExpandDims_70017: int8_t - 1,25,1,16


 int8_t* sequential_max_pooling1d_1_ExpandDims_70017 = sequential_conv1d_1_Relu_sequential_conv1d_1_BiasAdd_sequential_conv1d_1_Conv1D_Squeeze_sequential_conv1d_1_BiasAdd_ReadVariableOp_sequential_conv1d_1_Conv1D_70016;





//
// CMSIS-NN qMaxPool2d
//
{
cmsis_nn_pool_params pool_params = {
  .stride.h = 2,
  .stride.w = 1,
  .padding.h = 0,
  .padding.w = 0,
  .activation.min = -128,
  .activation.max = 127
};

cmsis_nn_dims input_dims = {
  .n = 1,
  .h = 25,
  .w = 1,
  .c = 16
};

cmsis_nn_dims filter_dims = {
  .n = 1,
  .h = 2,
  .w = 1,
  .c = 1
};

cmsis_nn_dims output_dims = {
  .n = 1,
  .h = 13,
  .w = 1,
  .c = 16
};

cmsis_nn_context ctx = {
  .buf = 0,
  .size = 0 // "size" is not currently used my CMSIS-NN
};

arm_max_pool_s8(
  &ctx,
  &pool_params,
  &input_dims,
  sequential_max_pooling1d_1_ExpandDims_70017,
  &filter_dims,
  &output_dims,
  sequential_max_pooling1d_1_MaxPool_70018
);
}


//
// Identity - bypassing sequential_flatten_Reshape_70019 operation
//
// Input sequential_max_pooling1d_1_MaxPool_70018: int8_t - 1,13,1,16
// Output sequential_flatten_Reshape_70019: int8_t - 1,208


 int8_t* sequential_flatten_Reshape_70019 = sequential_max_pooling1d_1_MaxPool_70018;






//
// CMSIS-NN fully connected
//
{
cmsis_nn_fc_params fc_params = {
  .input_offset = 128,
  .filter_offset = 0,
  .output_offset = 68,
  .activation.min = -128,
  .activation.max = 127
};

int32_t multiplier[1] = { 1662452790,  };

int32_t shift[1] = { -7,  };


cmsis_nn_quant_params quant_params = {
  .multiplier = multiplier,
  .shift = shift,
  .is_per_channel = false
};

cmsis_nn_dims input_dims = {
  .n = 1,
  .h = 1,
  .w = 208,
  .c = 1
};

cmsis_nn_dims filter_dims = {
  .n = 208,
  .h = 1,
  .w = 1,
  .c = 5
};

cmsis_nn_dims bias_dims = {
  .n = 1,
  .h = 1,
  .w = 1,
  .c = 5
};

cmsis_nn_dims output_dims = {
  .n = 1,
  .h = 1,
  .w = 1,
  .c = 5
};

#if defined(ARM_MATH_MVEI)
static const int32_t vector_sum[5] = { -26721, -9155, -740308, -30185, -51785,  };

ctx.buf = vector_sum;
#endif

arm_fully_connected_wrapper_s8(&ctx, &fc_params, &quant_params, &input_dims,
  sequential_flatten_Reshape_70019, &filter_dims, Int8VecConstant_70006, &bias_dims, Int32VecConstant_70005, &output_dims, sequential_y_pred_MatMul_sequential_y_pred_BiasAdd_70020);
}


//
// CMSIS-NN SoftMax
//

arm_softmax_s8(
  sequential_y_pred_MatMul_sequential_y_pred_BiasAdd_70020,
  1, // number of rows
  5, // row size
  1098398208, // input multiplier
  25, // input left shift
  -62, // input diff min
  StatefulPartitionedCall_0_70021
);


}
