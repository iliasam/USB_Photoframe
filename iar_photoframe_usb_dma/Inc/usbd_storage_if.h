#ifndef __USBD_STORAGE_IF_H_
#define __USBD_STORAGE_IF_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_msc.h"
#include "emfat.h"

extern USBD_StorageTypeDef  USBD_Storage_Interface_fops_FS;
extern emfat_t emfat;


#ifdef __cplusplus
}
#endif

#endif /* __USBD_STORAGE_IF_H_ */

