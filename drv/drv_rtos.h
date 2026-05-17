#ifndef DRV_RTOS_H_
#define DRV_RTOS_H_

/**
 * @file drv_rtos.h
 * @brief Vendor RTOS types - DRV-only header
 *
 * Only files in drv/ and ddi_mutex.c / ddi_semaphore.c (which need to map
 * slot enums to TX_MUTEX* / TX_SEMAPHORE*) are allowed to include this file.
 *
 * dca/ and app/ must NOT include this header - they only ever see the
 * ddi_mutex / ddi_semaphore slot APIs.
 */

#include "tx_api.h"   /* Azure RTOS ThreadX - vendor-supplied */

#endif /* DRV_RTOS_H_ */
