/*
 * Copyright (c) 2019 Stephen Ramm
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

/*!
 * \file    example1.c
 * \example example1
 * \brief   Use the image creation functions to create a white rectangle on a black background
 * and count the corners in the result using the Fast Corners function.
 * \author  Stephen Ramm <stephen.v.ramm@gmail.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <VX/vx.h>
#include <VX/vxu.h>

// 用于检查一个操作或对象是否创建或执行成功
void errorCheck(vx_context *context_p, vx_status status, const char *message)
{
    // 当status=0时，成功
    // 当status！=0时，失败
    if (status)
    {
      puts("ERROR! ");
      puts(message);
      // 释放context对象
      vxReleaseContext(context_p);
      exit(1);
    }
}

vx_image makeInputImage(vx_context context)
{
  // 生成一个100x100的8位图像
  vx_image image = vxCreateImage(context, 100U, 100U, VX_DF_IMAGE_U8);
  // 设置一个方形框对象，左上点坐标(20, 40),右下点坐标(80, 60)
  vx_rectangle_t rect = {
    .start_x = 20, .start_y = 40, .end_x=80, .end_y = 60
  };
  // 检查一下生成的vx_image对象是否成功
  if (VX_SUCCESS == vxGetStatus((vx_reference)image))
  {
    // 将rect覆盖到image上，生成一个roi
    // roi持有原始image的索引，roi的改变将导致原始image的改变
    vx_image roi = vxCreateImageFromROI(image, &rect);
    // 生成两个像素值对象
    vx_pixel_value_t pixel_white, pixel_black;
    pixel_white.U8 = 255; // 白色
    pixel_black.U8 = 0; // 黑色
    if (VX_SUCCESS == vxGetStatus((vx_reference)roi) &&
        VX_SUCCESS == vxSetImagePixelValues(image, &pixel_black) && // 将图像设置为黑色
        VX_SUCCESS == vxSetImagePixelValues(roi, &pixel_white)) // 将roi矩形设置为白色
      vxReleaseImage(&roi); // 释放roi对象
    else
      // 如果image对象生成失败，那么释放image
      vxReleaseImage(&image); // 释放image对象
  }

  // 返回image
  return image;
}

int main(void)
{
  // 生成context对象
  vx_context context = vxCreateContext();
  // 检测生成的context对象是否成功k
  errorCheck(&context, vxGetStatus((vx_reference)context), "Could not create a vx_context\n");
  // 生成一个100x100的image，并设置为黑色
  // 从中部生成一个60x20的矩形roi，并设置为白色
  vx_image image1 = makeInputImage(context);

  // 检查生成的image是否成功
  errorCheck(&context, vxGetStatus((vx_reference)image1), "Could not create image");

  vx_float32 strength_thresh_value = 128.0; // 用户与fast corner算法的强度阈值
  // 将强度阈值转换为vx_scalar类型，这样才能被vx的node所使用
  vx_scalar strength_thresh = vxCreateScalar(context, VX_TYPE_FLOAT32, &strength_thresh_value);
  // 生成vx_array类型，存储VX_TYPE_KEYPOINT类型对象，默认为100个
  vx_array corners = vxCreateArray(context, VX_TYPE_KEYPOINT, 100); // 接收使用nms后的关键点
  vx_array corners1 = vxCreateArray(context, VX_TYPE_KEYPOINT, 100); //  接收不使用nms后的关键点
  vx_size num_corners_value = 0; // 设置默认检测到的关键点数量
  vx_scalar num_corners = vxCreateScalar(context, VX_TYPE_SIZE, &num_corners_value); // 接收使用nms后的关键点数量
  vx_scalar num_corners1 = vxCreateScalar(context, VX_TYPE_SIZE, &num_corners_value); // 接收不使用用nms后的关键点
  // 分配可容纳100个关键点的内存，并设置为0，类型为vx_keypoint_t指针
  // malloc方法不会设置内存为0
  vx_keypoint_t *kp = calloc( 100, sizeof(vx_keypoint_t)); // 接收检测到的关键点向量拷贝，拥有输出

  errorCheck(&context,
             kp == NULL ||
             vxGetStatus((vx_reference)strength_thresh) ||
             vxGetStatus((vx_reference)corners) ||
             vxGetStatus((vx_reference)num_corners) ||
             vxGetStatus((vx_reference)corners1) ||
             vxGetStatus((vx_reference)num_corners1),
             "Could not create parameters for FastCorners");
  // 使用immediate mode调用fast corners算法，并设置使用nms，corners存储检测到的关键点向量，num_corners存储关键点数量
  errorCheck(&context, vxuFastCorners(context, image1, strength_thresh, vx_true_e, corners, num_corners), "Fast Corners function failed");
  // 使用immediate mode调用fast corners算法，并设置不使用nms，corners1存储检测到的关键点向量，num_corners1存储关键点数量
  errorCheck(&context, vxuFastCorners(context, image1, strength_thresh, vx_false_e, corners1, num_corners1), "Fast Corners function failed");
  // 拷贝num_corners到num_corners_value中，可以用于输出
  // VX_READ_ONLY： 将num_corners拷贝到num_corners_value
  // VX_WRITE_ONLY：将num_corners_value拷贝到num_corners
  errorCheck(&context, vxCopyScalar(num_corners, &num_corners_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST), "vxCopyScalar failed");
  printf("Found %zu corners with non-max suppression\n", num_corners_value);
  // 将corners拷贝到kp对应的内存，用于输出
  // VX_READ_ONLY： 将corners拷贝到kp
  // VX_WRITE_ONLY：将kp拷贝到corners
  errorCheck(&context, vxCopyArrayRange( corners, 0, num_corners_value, sizeof(vx_keypoint_t), kp,
                                        VX_READ_ONLY, VX_MEMORY_TYPE_HOST), "vxCopyArrayRange failed");
  for (int i=0; i<num_corners_value; ++i)
  {
    // 输出所有关键点的x和y坐标
    printf("Entry %3d: x = %d, y = %d\n", i, kp[i].x, kp[i].y);
  }

  // 同上，输出没有经过nms的结果
  errorCheck(&context, vxCopyScalar(num_corners1, &num_corners_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST), "vxCopyScalar failed");
  printf("Found %zu corners without non-max suppression\n", num_corners_value);

  errorCheck(&context, vxCopyArrayRange( corners1, 0, num_corners_value, sizeof(vx_keypoint_t), kp,
                                        VX_READ_ONLY, VX_MEMORY_TYPE_HOST), "vxCopyArrayRange failed");
  for (int i=0; i<num_corners_value; ++i)
  {
    printf("Entry %3d: x = %d, y = %d\n", i, kp[i].x, kp[i].y);
  }

  free(kp);
  vxReleaseContext(&context); //  释放context
  return 0;
}
