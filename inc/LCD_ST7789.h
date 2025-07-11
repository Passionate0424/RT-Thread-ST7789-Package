/**
 * @file LCD_ST7789.h
 * @brief ST7789 LCD驱动 - RT-Thread适配版
 *
 * 本头文件为ST7789 LCD屏幕的RT-Thread适配驱动接口声明。
 *
 * 主要内容：
 *   - LCD初始化与硬件配置
 *   - 基本/高级绘图API
 *   - 文本显示API
 *   - 方向、颜色、引脚等常用宏定义
 *
 * 适配说明：
 *   1. SPI通信与GPIO操作全部基于RT-Thread驱动框架
 *   2. 支持横屏/竖屏、DMA批量传输、快速填充等优化
 *   3. 兼容原裸机接口，便于移植
 *
 * 版本历史：
 *   v1.0  2025-07-11  初版整理
 */
#ifndef __LCD_ST7789_H__
#define __LCD_ST7789_H__

#include <rtthread.h>
// #include "font.h" /* 包含字体数据头文件 */
// #include <rtdevice.h>

//==================== LCD参数与硬件宏定义 ====================
#define BACK_COLOR BLACK // 默认背景色
#define LCD_W PKG_ST_7789_WIDTH        // 屏幕宽度(像素)
#define LCD_H PKG_ST_7789_HEIGHT        // 屏幕高度(像素)
#define FAST 1           // 快速刷图开关 1:快 0:慢

// 硬件引脚定义（需根据实际硬件修改）
#define LCD_DC_PIN PKG_ST_7789_DC_PIN  // 数据/命令选择
#define LCD_RES_PIN PKG_ST_7789_RES_PIN  // 复位
#define LCD_BLK_PIN PKG_ST_7789_BLK_PIN  // 背光
#define LCD_CS_PIN PKG_ST_7789_CS_PIN   // SPI片选
#define LCD_SPI_BUS PKG_ST_7789_SPI_BUS_NAME         // SPI总线名

// 引脚控制宏
#define LCD_RES_CLR rt_pin_write(LCD_RES_PIN, PIN_LOW)
#define LCD_RES_SET rt_pin_write(LCD_RES_PIN, PIN_HIGH)
#define LCD_DC_CLR rt_pin_write(LCD_DC_PIN, PIN_LOW)
#define LCD_DC_SET rt_pin_write(LCD_DC_PIN, PIN_HIGH)
#define LCD_BLK_CLR rt_pin_write(LCD_BLK_PIN, PIN_HIGH) // 背光低电平点亮
#define LCD_BLK_SET rt_pin_write(LCD_BLK_PIN, PIN_LOW)
#define DELAY(ms) rt_thread_mdelay(ms)

//==================== 类型定义与方向宏 ======================
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// 扫描方向定义
#define L2R_U2D 0 // 从左到右,从上到下
#define L2R_D2U 1 // 从左到右,从下到上
#define R2L_U2D 2 // 从右到左,从上到下
#define R2L_D2U 3 // 从右到左,从下到上
#define U2D_L2R 4 // 从上到下,从左到右
#define U2D_R2L 5 // 从上到下,从右到左
#define D2U_L2R 6 // 从下到上,从左到右
#define D2U_R2L 7 // 从下到上,从右到左

#define PORTRAIT U2D_R2L  // 竖屏方向
#define LANDSCAPE L2R_U2D // 横屏方向
#define Landscape 1       // 1:横屏 0:竖屏
extern u8 DFT_SCAN_DIR;   // 当前扫描方向

//==================== 颜色定义 ==============================
#define WHITE 0xFFFF
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0

//==================== 结构体定义 ============================
/**
 * @brief LCD设备参数结构体
 */
typedef struct
{
    u16 width;  // LCD宽度
    u16 height; // LCD高度
    u8 dir;     // 横屏/竖屏
    u8 wramcmd; // 写GRAM指令
    u8 setxcmd; // 设置X坐标指令
    u8 setycmd; // 设置Y坐标指令
} _lcd_dev;
extern _lcd_dev lcddev;

//==================== 方向切换API ===========================
void LCD_SetPortrait(void);  // 设置为竖屏
void LCD_SetLandscape(void); // 设置为横屏

//==================== LCD初始化与配置API ====================
int spi_lcd_init(void);       // LCD驱动初始化
void LCD_Init(void);          // LCD初始化
void LCD_Display_Dir(u8 dir); // 设置显示方向
void LCD_Scan_Dir(u8 dir);    // 设置扫描方向

//==================== 基本绘图API ===========================
void LCD_Clear(u16 Color);
void LCD_SetCursor(u16 Xpos, u16 Ypos);
void LCD_DrawPoint(u16 x, u16 y, u16 color);

//==================== 高级绘图API ===========================
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void Draw_Circle(u16 x0, u16 y0, u8 r, u16 color);
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);
void LCD_ShowImage(u16 x, u16 y, u16 width, u16 height, const u16 *p);
void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color);

//==================== 文本显示API ===========================
void LCD_ShowChar(u16 x, u16 y, char chr, u8 size, u8 mode, u16 color, u16 bg_color);
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p, u16 color, u16 bg_color);
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size, u16 color, uint16_t bg_color);
void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode, u16 color, uint16_t bg_color);

//==================== 辅助功能API ===========================
u32 LCD_Pow(u8 m, u8 n);

#endif /* __LCD_ST7789_H__ */
