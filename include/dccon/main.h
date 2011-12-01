#ifndef _MAIN_
#define _MAIN_

#include "drv_ioctl.h"

void  cls_console();
char  getchr(char min, char max);
char* dc_get_password(int confirm);
int   dc_set_boot_interactive(int d_num);

#define on_off(a) ( (a) != 0 ? L"ON":L"OFF" )
#define set_flag(var,flag,value) if ((value) == 0) { (var) &= ~(flag); } else { (var) |= (flag); }

#define MAX_VOLUMES 128

extern vol_inf volumes[MAX_VOLUMES];
extern u32     vol_cnt;

#endif