/**
 ****************************************************************************************************
 * @file        malloc.c
 * @author      正点原子(ALIENTEK)
 * @version     V1.0
 * @date        2021-11-04
 * @brief       内存管理
 * @license     Copyright (c) 2020-2032, 正点原子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台: 正点原子 STM32系列
 * 公司官网: www.alientek.com
 * 技术论坛: www.openedv.com
 *
 * 修改说明:
 * V1.0 20211104
 * 初次发布
 *
 ****************************************************************************************************
 */

#include "./MALLOC/malloc.h"

/* 内存池(32字节对齐) */
__align(32) uint8_t  mem1base[MEM1_MAX_SIZE];                                                   /* 内部SRAM内存池 */
__align(32) uint8_t  mem2base[MEM2_MAX_SIZE] __attribute__((at(0x10000000)));                   /* 内部CCM内存池 */
__align(32) uint8_t  mem3base[MEM3_MAX_SIZE] __attribute__((at(0x68000000)));                   /* 外部SRAM内存池 */

/* 内存管理表 */
uint16_t  mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                   /* 内部SRAM内存池MAP */
uint16_t  mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0x10000000 + MEM2_MAX_SIZE)));   /* 内部CCM内存池MAP */
uint16_t  mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((at(0x68000000 + MEM3_MAX_SIZE)));   /* 外部SRAM内存池MAP */

/* 内存管理参数 */
const uint32_t  memtblsize[SRAMBANK] = {MEM1_ALLOC_TABLE_SIZE, MEM2_ALLOC_TABLE_SIZE, MEM3_ALLOC_TABLE_SIZE};   /* 内存表大小 */
const uint32_t  memblksize[SRAMBANK] = {MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE};                     /* 内存块大小 */
const uint32_t  memsize[SRAMBANK]    = {MEM1_MAX_SIZE, MEM2_MAX_SIZE, MEM3_MAX_SIZE};                           /* 内存总大小 */

/* 内存管理控制器 */
struct _m_mallco_dev mallco_dev=
{
    my_mem_init,                            /* 内存初始化 */
    my_mem_perused,                         /* 内存使用率 */
    mem1base, mem2base, mem3base,           /* 内存池 */
    mem1mapbase, mem2mapbase, mem3mapbase,  /* 内存管理状态表 */
    0,0,0,                                  /* 内存未就绪 */
};

/**
 * @brief       内存拷贝
 * @param       *des : 目的地址
 * @param       *src : 源地址
 * @param       n    : 要拷贝的字节数
 * @retval      无
 */
void mymemcpy(void *des, void *src, uint32_t n)
{
    uint8_t  *xdes = des;
    uint8_t  *xsrc = src;
    while (n--)*xdes++ = *xsrc++;
}

/**
 * @brief       内存设置
 * @param       *s    : 内存起始地址
 * @param       c     : 要设置的值
 * @param       count : 要设置的字节数
 * @retval      无
 */
void mymemset(void *s,uint8_t c, uint32_t count)
{
    uint8_t  *xs = s;
    while( count-- )*xs++ = c;
}

/**
 * @brief       内存管理初始化
 * @param       memx : 内存编号
 * @retval      无
 */
void my_mem_init(uint8_t  memx)
{
    mymemset(mallco_dev.memmap[memx], 0,memtblsize[memx] * 2);  /* 内存状态表清零 */
    mymemset(mallco_dev.membase[memx], 0,memsize[memx]);        /* 内存池清零 */
    mallco_dev.memrdy[memx] = 1;                                /* 内存初始化OK */
}

/**
 * @brief       获取内存使用率
 * @param       memx : 内存编号
 * @retval      使用率(放大10倍,0~1000,对应0.0%~100.0%)
 */
uint16_t  my_mem_perused(uint8_t memx)
{
    uint32_t  used = 0;
    uint32_t  i;
    for (i = 0; i < memtblsize[memx]; i++)
    {
        if (mallco_dev.memmap[memx][i])
        {
            used++;
        }
    }
    return (used * 1000) / (memtblsize[memx]);
}

/**
 * @brief       内存分配(内部调用)
 * @param       memx : 内存编号
 * @param       size : 要分配的内存大小(字节)
 * @retval      内存偏移地址
 *   @arg       0 ~ 0XFFFFFFFE : 有效偏移地址
 *   @arg       0XFFFFFFFF     : 分配失败
 */
uint32_t  my_mem_malloc(uint8_t memx, uint32_t size)
{
    signed long offset = 0;
    uint32_t  nmemb;        /* 需要的内存块数 */
    uint32_t  cmemb = 0;    /* 连续空闲块数 */
    uint32_t  i;
    
    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);  /* 未初始化,先执行初始化 */
    }
    if(size == 0)
    {
        return 0xFFFFFFFF;      /* 无需分配 */
    }
    
    nmemb = size / memblksize[memx];                            /* 计算需要多少个内存块 */
    if (size % memblksize[memx])
    {
        nmemb++;
    }
    for (offset = memtblsize[memx] - 1; offset >= 0; offset--)  /* 从末尾查找 */
    {
        if (!mallco_dev.memmap[memx][offset])
        {
            cmemb++;        /* 连续空闲块计数 */
        }
        else 
        {
            cmemb = 0;      /* 遇到已占用块,清零 */
        }
        if (cmemb == nmemb) /* 找到连续nmemb个空闲块 */
        {
            for (i = 0; i < nmemb; i++)                         /* 标记为已占用 */
            {
                mallco_dev.memmap[memx][offset + i] = nmemb;
            }
            return (offset * memblksize[memx]);                 /* 返回偏移地址 */
        }
    }
    return 0XFFFFFFFF;      /* 未找到足够连续内存 */
}

/**
 * @brief       内存释放(内部调用)
 * @param       memx   : 内存编号
 * @param       offset : 内存地址偏移
 * @retval      释放结果
 *   @arg       0, 释放成功;
 *   @arg       1, 未初始化;
 *   @arg       2, 偏移越界;
 */
uint8_t  my_mem_free(uint8_t memx, uint32_t offset)
{
    int i;
    
    if (!mallco_dev.memrdy[memx])   /* 未初始化,先初始化 */
    {
        mallco_dev.init(memx);
        return 1;                   /* 未初始化 */
    }
    if (offset < memsize[memx])     /* 偏移在内存范围内 */
    {
        int index = offset / memblksize[memx];      /* 计算对应块索引 */
        int nmemb = mallco_dev.memmap[memx][index]; /* 获取占用块数 */
        for (i = 0; i < nmemb; i++)                 /* 释放对应块 */
        {
            mallco_dev.memmap[memx][index + i] = 0;
        }
        return 0;
    }
    else
    {
        return 2;                   /* 偏移越界 */
    }
}

/**
 * @brief       内存释放(外部调用)
 * @param       memx : 内存编号
 * @param       ptr  : 内存指针
 * @retval      无
 */
void myfree(uint8_t memx, void *ptr)
{
    uint32_t  offset;
    
    if (ptr == NULL)
    {
        return;                 /* 空指针直接返回 */
    }
    offset = (uint32_t)ptr - (uint32_t )mallco_dev.membase[memx];
    my_mem_free(memx, offset);  /* 释放内存 */
}

/**
 * @brief       内存分配(外部调用)
 * @param       memx : 内存编号
 * @param       size : 要分配的内存大小(字节)
 * @retval      分配到的内存指针.
 */
void *mymalloc(uint8_t memx, uint32_t size)
{
    uint32_t  offset;
    
    offset = my_mem_malloc(memx, size);

    if (offset == 0XFFFFFFFF)
    {
        return NULL;
    }
    else
    {
        return (void*)((uint32_t)mallco_dev.membase[memx] + offset);
    }
}

/**
 * @brief       重新分配内存(外部调用)
 * @param       memx : 内存编号
 * @param       *ptr : 旧内存指针
 * @param       size : 新内存大小(字节)
 * @retval      新内存指针.
 */
void *myrealloc(uint8_t  memx, void *ptr, uint32_t size)
{
    uint32_t  offset;
    
    offset = my_mem_malloc(memx, size);
    if(offset == 0XFFFFFFFF)
    {
        return NULL;
    }
    else
    {
        mymemcpy((void*)((uint32_t )mallco_dev.membase[memx] + offset), ptr, size); /* 拷贝旧数据到新内存 */
        myfree(memx, ptr);                                                          /* 释放旧内存 */
        return (void*)((uint32_t )mallco_dev.membase[memx] + offset);               /* 返回新内存指针 */
    }
}