#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#define STRINGIFY(v) #v
#define EVAL_STRINGIFY(v) STRINGIFY(v)

#define PRODUCT_NAME_RAW Media Player Volume Control
#define PRODUCT_NAME EVAL_STRINGIFY(PRODUCT_NAME_RAW)

#undef CREATEPROCESS_MANIFEST_RESOURCE_ID
#define CREATEPROCESS_MANIFEST_RESOURCE_ID 2
#define IDI_ICON 101

#define IDR_TRAY_POPUPMENU 110
#define IDM_TRAY_POPUPMENU_TOGGLE 111
#define IDM_TRAY_POPUPMENU_EXIT 112

#endif // __RESOURCE_H__
