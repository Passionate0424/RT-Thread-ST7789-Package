# LCD_ST7789 软件包 API 文档

## 1. 结构体与全局变量

- `_lcd_dev`  
  LCD设备描述结构体，包含屏幕宽高、方向、写入命令等信息。
  ```c
  typedef struct {
      u16 width;      // LCD宽度
      u16 height;     // LCD高度
      u8 dir;         // 横屏/竖屏
      u8 wramcmd;     // 写GRAM指令
      u8 setxcmd;     // 设置X坐标指令
      u8 setycmd;     // 设置Y坐标指令
  } _lcd_dev;
  ```
- `extern _lcd_dev lcddev;`  
  全局LCD设备实例。

- `extern u8 DFT_SCAN_DIR;`  
  当前扫描方向。

## 2. 初始化与配置

- `int spi_lcd_init(void);`  
  SPI LCD设备注册与初始化（RT-Thread自动调用）。

- `void LCD_Init(void);`  
  LCD初始化。

- `void LCD_Display_Dir(u8 dir);`  
  设置显示方向（0竖屏，1横屏）。

- `void LCD_Scan_Dir(u8 dir);`  
  设置扫描方向（0~7）。

- `void LCD_SetPortrait(void);`  
  快速切换为竖屏。

- `void LCD_SetLandscape(void);`  
  快速切换为横屏。

## 3. 基本绘图函数

- `void LCD_Clear(u16 Color);`  
  清屏，填充指定颜色。

- `void LCD_SetCursor(u16 Xpos, u16 Ypos);`  
  设置光标位置。

- `void LCD_DrawPoint(u16 x, u16 y, u16 color);`  
  画点。

- `void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);`  
  画线。

- `void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);`  
  画矩形。

- `void Draw_Circle(u16 x0, u16 y0, u8 r, u16 color);`  
  画圆。

- `void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);`  
  区域填充。

- `void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color);`  
  区域彩色填充。

- `void LCD_ShowImage(u16 x, u16 y, u16 width, u16 height, const u16 *p);`  
  显示图片（RGB565）。

## 4. 文本显示函数

- `void LCD_ShowChar(u16 x, u16 y, char chr, u8 size, u8 mode, u16 color, u16 bg_color);`  
  显示单个字符。

- `void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p, u16 color, u16 bg_color);`  
  显示字符串。

- `void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size, u16 color, uint16_t bg_color);`  
  显示数字。

- `void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode, u16 color, uint16_t bg_color);`  
  显示数字（带模式）。

## 5. 字体

- `extern const unsigned char asc2_1206[95][12];`  
- `extern const unsigned char asc2_1608[95][16];`  
  常用ASCII字模，支持12x6和16x8两种点阵。
