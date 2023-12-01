/**
 * @file tkl_memory.c
 * @brief 内存操作接口封装
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"

#include "tkl_memory.h"

/**
 * @brief tkl_system_malloc 用于分配内存
 * 
 * @param[in]       size        需要分配的内存大小
 * @return  分配得到的内存指针
 */
void *tkl_system_malloc(const size_t size)
{
    void *pMalloc;
    pMalloc = pvPortMalloc(size);
    return pMalloc;
}

/**
 * @brief tkl_system_free 用于释放内存
 * 
 * @param[in]       ptr         需要释放的内存指针
 */

void tkl_system_free(void* ptr)
{
    vPortFree(ptr);
}

VOID_T *tkl_system_calloc(size_t nitems, size_t size)
{
    void *addr;

    addr = tkl_system_malloc(nitems * size);
    if (addr == NULL) {
        return addr;
    }

    memset(addr, 0, nitems * size);
    return addr;
}

/**
 * @brief Re-allocate the memory
 * 
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return VOID_T
 */
VOID_T *tkl_system_realloc(VOID_T* ptr, size_t size)
{
    void *new;

    if (size == 0) {
        tkl_system_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return tkl_system_malloc(size);
    }

    new = tkl_system_malloc(size);
    if (new == NULL) {
        return NULL;
    }

    memcpy(new, ptr, size);
    tkl_system_free(ptr);
    return new;
}
