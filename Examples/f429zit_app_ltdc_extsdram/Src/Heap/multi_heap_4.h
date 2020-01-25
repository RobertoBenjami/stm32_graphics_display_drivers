/*
 * multi_heap_4.h
 *
 *  Created on: Oct 13, 2019
 *      Author: Benjami
 */

/*
 * If used freertos: the original function call can also be used for compatibility
 * (for the internal sram memory region: pvPortMalloc, vPortFree, xPortGetFreeHeapSize)
 */

#ifndef __MULTI_HEAP_4_H_
#define __MULTI_HEAP_4_H_

#ifdef __cplusplus
 extern "C" {
#endif

/*-----------------------------------------------------------------------------
  Parameter section (set the parameters)
*/

/*
 * Please setting:
 * - HEAP_NUM : how many heap ram regions are
 * - configTOTAL_HEAP_SIZE : internal sram region heap size (in the freertos is predefinied)
 * - HEAP_REGIONS : memory address and size for regions
 *
 * This example contains 3 regions (stm32f407zet board, external 1MB sram on FSMC)
 * - 0: default memory region (internal sram) -> iramMalloc, iramFree, iramGetFreeHeapSize
 * - 1: ccm internal 64kB ram (0x10000000..0x1000FFFF) -> cramMalloc, cramFree, cramGetFreeHeapSize
 * - 2: external 1MB sram (0x68000000..0x680FFFFF) -> eramMalloc, eramFree, eramGetFreeHeapSize
 *   #define HEAP_REGIONS  {{ (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                            { (uint8_t *) 0x10000000, 0x10000       },\
                            { (uint8_t *) 0x68000000, 0x100000      }};
 *
 * This example contains 3 regions (stm32f429zit discovery, 8MB sdram on FSMC)
 * - 0: default memory region (internal sram) -> iramMalloc, iramFree, iramGetFreeHeapSize
 * - 1: ccm internal 64kB ram (0x10000000..0x1000FFFF) -> cramMalloc, cramFree, cramGetFreeHeapSize
 * - 2: external 8MB sdram (0xD0000000..0xD07FFFFF) -> eramMalloc, eramFree, eramGetFreeHeapSize
 *   #define HEAP_REGIONS  {{ (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                            { (uint8_t *) 0x10000000, 0x10000       },\
                            { (uint8_t *) 0xD0000000, 0x800000      }};
*/

/* Heap region number (1..6) */
#define HEAP_NUM      3

/* region 0 heap static reservation (if not used freertos -> check the free RAM size for setting) */
#ifndef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE    0x20000
#endif

#define HEAP_0        ucHeap0[configTOTAL_HEAP_SIZE] // internal sram heap reservation if freertos used

/* regions table: address and size (internal sram region (0), ccmram region (1), external ram region (2) ) */
#define HEAP_REGIONS  {{ (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                       { (uint8_t *) 0x10000000, 0x10000       },\
                       { (uint8_t *) 0xD0000000, 0x800000      }};

/* internal sram memory (region 0) procedures (*iramMalloc, iramFree, iramGetFreeHeapSize) */
#define iramMalloc(a)        multiPortMalloc(0, a)
#define iramFree(a)          multiPortFree(0, a)
#define iramGetFreeHeapSize(a)  multiPortGetFreeHeapSize(0)
#define iramGetMinimumEverFreeHeapSize multiPortGetMinimumEverFreeHeapSize(0)

/* ccm ram memory (region 1) procedures (*cramMalloc, cramFree, cramGetFreeHeapSize) */
#define cramMalloc(a)        multiPortMalloc(1, a)
#define cramFree(a)          multiPortFree(1, a)
#define cramGetFreeHeapSize(a)  multiPortGetFreeHeapSize(1)
#define cramGetMinimumEverFreeHeapSize multiPortGetMinimumEverFreeHeapSize(1)

/* external ram memory (region 2) procedures (*eramMalloc, eramFree, eramGetFreeHeapSize) */
#define eramMalloc(a)      multiPortMalloc(2, a)
#define eramFree(a)        multiPortFree(2, a)
#define eramGetFreeHeapSize(a)  multiPortGetFreeHeapSize(2)
#define eramGetMinimumEverFreeHeapSize multiPortGetMinimumEverFreeHeapSize(2)

/*-----------------------------------------------------------------------------
  Fix section, do not change
*/
void*   multiPortMalloc(uint32_t i, size_t xWantedSize);
void    multiPortFree(uint32_t i, void *pv);
size_t  multiPortGetFreeHeapSize(uint32_t i);
size_t  multiPortGetMinimumEverFreeHeapSize(uint32_t i);

#ifdef __cplusplus
}
#endif

#endif /* __MULTI_HEAP_4_H_ */
