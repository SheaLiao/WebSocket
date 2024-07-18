/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<li@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  mutex.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(16/07/24)
 *         Author:  Liao Shengli <li@gmail.com>
 *      ChangeLog:  1, Release initial version on "16/07/24 20:17:03"
 *                 
 ********************************************************************************/

#include "mutex.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 初始化全局互斥锁

