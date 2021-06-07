/****************************************************************************
 *                                                                          *
 * Copyright (c) 2018 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ***************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     main.c
 *
 * VERSION
 *     1.1
 *
 * DESCRIPTION
 *     To utilize emWin library to demonstrate  widgets feature.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *     2019/01/08        Ver 1.1 Updated
 *
 * REMARK
 *     None
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "stdint.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/time.h>

#include <pthread.h>  // Thread

#include "GUI.h"
//#include "LCDConf.h"
#include "WM.h"
/*IOCTLs*/
//#define IOCTLSETCURSOR            _IOW('v', 21, unsigned int) //set cursor position
#define VIDEO_ACTIVE_WINDOW_COORDINATES     _IOW('v', 22, unsigned int) //set display-start line in display buffer
//#define IOCTLREADSTATUS           _IOW('v', 23, unsigned int) //read lcd module status
#define VIDEO_DISPLAY_ON            _IOW('v', 24, unsigned int) //display on
#define VIDEO_DISPLAY_OFF           _IOW('v', 25, unsigned int) //display off
//#define IOCTLCLEARSCREEN      _IOW('v', 26, unsigned int) //clear screen
//#define VIDEO_UPSCALING           _IOW('v', 27, unsigned int) //video up scaling
#define IOCTL_LCD_BRIGHTNESS        _IOW('v', 27, unsigned int)  //brightness control



#define VIDEO_DISPLAY_LCD           _IOW('v', 38, unsigned int) //display on
#define VIDEO_DISPLAY_TV            _IOW('v', 39, unsigned int) //display off
#define VIDEO_FORMAT_CHANGE         _IOW('v', 50, unsigned int) //frame buffer format change
#define VIDEO_TV_SYSTEM             _IOW('v', 51, unsigned int) //set TV NTSC/PAL system

//#define LCD_RGB565_2_RGB555       _IOW('v', 30, unsigned int) //RGB565_2_RGB555
//#define LCD_RGB555_2_RGB565       _IOW('v', 31, unsigned int) //RGB555_2_RGB565

#define LCD_RGB565_2_RGB555     _IO('v', 30)
#define LCD_RGB555_2_RGB565     _IO('v', 31)

#define LCD_ENABLE_INT      _IO('v', 28)
#define LCD_DISABLE_INT     _IO('v', 29)

//#define IOCTL_LCD_GET_DMA_BASE          _IOR('v', 32, unsigned int *)

#define DISPLAY_MODE_RGB555 0
#define DISPLAY_MODE_RGB565 1
#define DISPLAY_MODE_CBYCRY 4
#define DISPLAY_MODE_YCBYCR 5
#define DISPLAY_MODE_CRYCBY 6
#define DISPLAY_MODE_YCRYCB 7
/* Macros about LCM */
#define CMD_DISPLAY_ON                      0x3F
#define CMD_DISPLAY_OFF                     0x3E
#define CMD_SET_COL_ADDR                    0x40
#define CMD_SET_ROW_ADDR                    0xB8
#define CMD_SET_DISP_START_LINE     0xC0


#define CHAR_WIDTH      5

struct fb_var_screeninfo var;
struct fb_fix_screeninfo finfo;
unsigned short *pVideoBuffer;
//unsigned char *g_VAFrameBuf;
int g_xres;
int g_yres;
int g_bits_per_pixel;

typedef struct Cursor
{
    unsigned char x;
    unsigned char y;
} Cursor;

typedef struct
{
    unsigned int start;
    unsigned int end;
} ActiveWindow;

typedef struct
{
    int hor;
    int ver;
} video_scaling;

typedef  struct _font
{
    unsigned char c[CHAR_WIDTH];
} font;

/* Dot-matrix of 0,1,2,3,4,5,6,7,8,9 */
font myFont[11] = {{0x3e, 0x41, 0x41, 0x3e, 0x00}, //zero
    {0x00, 0x41, 0x7f, 0x40, 0x00}, //un
    {0x71, 0x49, 0x49, 0x46, 0x00}, //deux
    {0x49, 0x49, 0x49, 0x36, 0x00}, //trois
    {0x0f, 0x08, 0x08, 0x7f, 0x00}, //quatre
    {0x4f, 0x49, 0x49, 0x31, 0x00}, //cinq
    {0x3e, 0x49, 0x49, 0x32, 0x00}, //six
    {0x01, 0x01, 0x01, 0x7f, 0x00}, //sept
    {0x36, 0x49, 0x49, 0x36, 0x00}, //huit
    {0x06, 0x49, 0x49, 0x3e, 0x00}, //neuf
    {0x00, 0x00, 0x60, 0x00, 0x00}
}; // point
int fd;
extern void MainTask(void);
extern void TouchTask(void);
void log_out(const char * s)
{
    printf("%s",s);
}
void *MainTask_ISR(void *arg)
{
    printf("Main Task thread\n");
    GUI_SetOnErrorFunc(&log_out);
    //BenchTask();
    MainTask();
}
void FlushTask(void *arg)
{
    usleep(40000);
    ioctl(fd, FBIOPAN_DISPLAY, &var);
    // while (1)
    // {
    //     ioctl(fd, FBIOPAN_DISPLAY, &var);
    //     usleep(40000);
    // }

}
void *TouchTask_ISR(void *arg)
{
    printf("Touch Task thread\n");
    TouchTask();
}

int main()
{
    int  ret;
    int i, t = 0;

    FILE *fpVideoImg;

    unsigned long uVideoSize;

    pthread_t tid1, tid2,tid3;


    fd = open("/dev/fb0", O_RDWR);
    if (fd == -1)
    {
        printf("Cannot open fb0!\n");
        return -1;
    }
    if(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        close(fd);
        return -1;
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
        perror("ioctl FBIOGET_VSCREENINFO");
        close(fd);
        return -1;
    }

//  ioctl(fd,LCD_ENABLE_INT);
    uVideoSize = var.xres * var.yres * var.bits_per_pixel / 8;

    printf("uVideoSize = 0x%x\n", uVideoSize);
    printf("var.xres = 0x%x\n", var.xres);
    printf("var.yres = 0x%x\n", var.yres);
    printf("var.xoffset = 0x%x\n", var.xoffset);
    printf("var.yoffset = 0x%x\n", var.yoffset);
    printf("var.bits_per_pixel = 0x%x\n", var.bits_per_pixel);
    printf("finfo.line_length = 0x%x\n", finfo.line_length);
    //  printf("uVideoSize = 0x%x \n", uVideoSize);
    pVideoBuffer = mmap(NULL, uVideoSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    printf("pVideoBuffer = 0x%x\n", pVideoBuffer);
    if(pVideoBuffer == MAP_FAILED)
    {
        printf("LCD Video Map Failed!\n");
        exit(0);
    }
    //ioctl(fd, IOCTL_LCD_GET_DMA_BASE, &g_VAFrameBuf);
    // the processing of video buffer
    //g_VAFrameBuf = pVideoBuffer;
    g_xres = var.xres;
    g_yres = var.yres;
    g_bits_per_pixel = var.bits_per_pixel;

    // uint16_t *fbp16 = (unsigned short*)pVideoBuffer;
    // long int location = 0;
    // uint16_t testcolor=0xf800;
    // printf("start show !\n");
    // for (int y0=0; y0 <= 599; y0++) {

    //   long int location =  y0 * 1024;
    //   GUI__memset16(&fbp16[location], testcolor, (1024));
    // }
    ioctl(fd, FBIOPAN_DISPLAY, &var);
    pthread_create(&tid1, NULL, MainTask_ISR, (void *)"MainTask");
    //pthread_create(&tid3, NULL, FlushTask, (void *)"MainTask");
    //pthread_create(&tid2, NULL, TouchTask_ISR, (void *)"TouchTask");

    pthread_join(tid1, NULL);
    //pthread_join(tid3, NULL);
    //pthread_join(tid2, NULL);

//    MainTask();

    /* Close LCD */
    //ioctl(fd, VIDEO_DISPLAY_OFF);
    munmap(pVideoBuffer, uVideoSize);  //return memory
    close(fd);
    return 0;
}
