# LCD_ST7789 软件包 API 文档（详细版）

## 1. 结构体与全局变量

- `_lcd_dev`  
  LCD 设备描述结构体，包含屏幕宽高、方向、写入命令等信息。
  ```c
  typedef struct {
      u16 width;      // LCD宽度（像素）
      u16 height;     // LCD高度（像素）
      u8 dir;         // 屏幕方向（0竖屏，1横屏）
      u8 wramcmd;     // 写GRAM指令
      u8 setxcmd;     // 设置X坐标指令
      u8 setycmd;     // 设置Y坐标指令
  } _lcd_dev;
  ```
  - width/height：当前屏幕的宽高，随方向切换自动调整。
  - dir：0为竖屏，1为横屏。
  - wramcmd/setxcmd/setycmd：底层寄存器命令，通常无需手动修改。

- `extern _lcd_dev lcddev;`  
  全局 LCD 设备实例，所有操作均基于此结构体。

- `extern u8 DFT_SCAN_DIR;`  
  当前扫描方向，影响内容显示方向。

## 2. 初始化与配置

- `int spi_lcd_init(void);`
  - 功能：SPI LCD 设备注册与初始化（RT-Thread 自动调用）。
  - 返回值：0 成功，负值失败。
  - 参数：无。
  - 注意：通常无需手动调用，由 RT-Thread 启动自动完成。
  - 示例：
    ```c
    int ret = spi_lcd_init();
    if(ret != 0) {
        // 初始化失败处理
    }
    ```

- `void LCD_Init(void);`
  - 功能：LCD 初始化（兼容旧接口，推荐用 spi_lcd_init）。
  - 参数：无。
  - 示例：
    ```c
    LCD_Init();
    ```

- `void LCD_Display_Dir(u8 dir);`
  - 功能：设置显示方向。
  - 参数：
    - dir：0 竖屏，1 横屏。
  - 会自动调整 lcddev.width/height。
  - 示例：
    ```c
    LCD_Display_Dir(1); // 设置为横屏
    ```

- `void LCD_Scan_Dir(u8 dir);`
  - 功能：设置扫描方向。
  - 参数：
    - dir：0~7，详见 LCD_ST7789.h 宏定义。
  - 示例：
    ```c
    LCD_Scan_Dir(0); // 默认扫描方向
    ```

- `void LCD_SetPortrait(void);`
  - 功能：快速切换为竖屏。
  - 参数：无。
  - 示例：
    ```c
    LCD_SetPortrait();
    ```

- `void LCD_SetLandscape(void);`
  - 功能：快速切换为横屏。
  - 参数：无。
  - 示例：
    ```c
    LCD_SetLandscape();
    ```

## 3. 基本绘图函数

- `void LCD_Clear(u16 Color);`
  - 功能：清屏，填充指定颜色。
  - 参数：
    - Color：填充颜色，RGB565 格式。
  - 示例：
    ```c
    LCD_Clear(BLACK);
    ```

- `void LCD_SetCursor(u16 Xpos, u16 Ypos);`
  - 功能：设置光标位置。
  - 参数：
    - Xpos：X 坐标。
    - Ypos：Y 坐标。
  - 示例：
    ```c
    LCD_SetCursor(10, 20);
    ```

- `void LCD_DrawPoint(u16 x, u16 y, u16 color);`
  - 功能：在指定坐标画点。
  - 参数：
    - x：X 坐标。
    - y：Y 坐标。
    - color：点的颜色，RGB565 格式。
  - 示例：
    ```c
    LCD_DrawPoint(50, 50, RED);
    ```

- `void LCD_DrawPoints(u16 *points_x, u16 *points_y, u16 point_count, u16 color);`
  - 功能：批量绘制多个点，适合轨迹、散点等高效渲染。
  - 参数：
    - points_x：X 坐标数组指针。
    - points_y：Y 坐标数组指针。
    - point_count：点的数量。
    - color：点的颜色，RGB565 格式。
  - 示例：
    ```c
    u16 xs[5] = {10, 20, 30, 40, 50};
    u16 ys[5] = {10, 20, 30, 40, 50};
    LCD_DrawPoints(xs, ys, 5, RED);
    ```

- `void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);`
  - 功能：画线，支持任意方向。
  - 参数：
    - x1, y1：起点坐标。
    - x2, y2：终点坐标。
    - color：线的颜色，RGB565 格式。
  - 示例：
    ```c
    LCD_DrawLine(0, 0, 100, 100, GREEN);
    ```

- `void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);`
  - 功能：画矩形边框。
  - 参数：
    - x1, y1：左上角坐标。
    - x2, y2：右下角坐标。
    - color：边框颜色，RGB565 格式。
  - 示例：
    ```c
    LCD_DrawRectangle(10, 10, 60, 60, YELLOW);
    ```

- `void Draw_Circle(u16 x0, u16 y0, u8 r, u16 color);`
  - 功能：画圆。
  - 参数：
    - x0, y0：圆心坐标。
    - r：半径。
    - color：圆的颜色，RGB565 格式。
  - 示例：
    ```c
    Draw_Circle(50, 50, 20, CYAN);
    ```

- `void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);`
  - 功能：区域填充单色。
  - 参数：
    - sx, sy：起始坐标。
    - ex, ey：结束坐标。
    - color：填充颜色，RGB565 格式。
  - 示例：
    ```c
    LCD_Fill(20, 20, 100, 100, BLUE);
    ```

- `void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color);`
  - 功能：区域彩色填充。
  - 参数：
    - sx, sy：起始坐标。
    - ex, ey：结束坐标。
    - color：颜色数组指针，长度为 (ex-sx+1)*(ey-sy+1)。
  - 示例：
    ```c
    u16 color_buf[100];
    // 填充 color_buf ...
    LCD_Color_Fill(0, 0, 9, 9, color_buf);
    ```

- `void LCD_ShowImage(u16 x, u16 y, u16 width, u16 height, const u16 *p);`
  - 功能：显示图片（RGB565 格式）。
  - 参数：
    - x, y：图片左上角坐标。
    - width, height：图片宽高。
    - p：图片数据指针，RGB565 格式，行优先。
  - 示例：
    ```c
    extern const u16 img_data[240*240];
    LCD_ShowImage(0, 0, 240, 240, img_data);
    ```

- `void LCD_DispFlush(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint16_t *pData);`
  - 功能：区域刷屏，适用于 LVGL 等 GUI 框架的显存刷新。
  - 参数：
    - x1, y1：区域左上角坐标。
    - x2, y2：区域右下角坐标。
    - pData：像素数据指针，RGB565 格式，长度为 (x2-x1+1)*(y2-y1+1)。
  - 示例：
    ```c
    extern uint16_t buf[100*100];
    LCD_DispFlush(0, 0, 99, 99, buf);
    ```

## 4. 文本显示函数

- `void LCD_ShowChar(u16 x, u16 y, char chr, u8 size, u8 mode, u16 color, u16 bg_color);`
  - 功能：显示单个字符。
  - 参数：
    - x, y：字符左上角坐标。
    - chr：要显示的字符。
    - size：字体大小（12/16）。
    - mode：0 覆盖模式，1 透明模式。
    - color：字体颜色。
    - bg_color：背景色（透明模式下无效）。
  - 示例：
    ```c
    LCD_ShowChar(10, 10, 'A', 16, 0, RED, WHITE);
    ```

- `void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p, u16 color, u16 bg_color);`
  - 功能：显示字符串。
  - 参数：
    - x, y：字符串起始坐标。
    - width, height：显示区域宽高。
    - size：字体大小（12/16）。
    - p：字符串指针。
    - color：字体颜色。
    - bg_color：背景色。
  - 示例：
    ```c
    LCD_ShowString(10, 30, 100, 20, 16, (u8*)"Hello LCD", RED, WHITE);
    ```

- `void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size, u16 color, uint16_t bg_color);`
  - 功能：显示无符号整数。
  - 参数：
    - x, y：数字起始坐标。
    - num：要显示的数字。
    - len：显示长度（位数）。
    - size：字体大小（12/16）。
    - color：字体颜色。
    - bg_color：背景色。
  - 示例：
    ```c
    LCD_ShowNum(10, 50, 12345, 5, 16, BLUE, WHITE);
    ```

- `void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode, u16 color, uint16_t bg_color);`
  - 功能：显示无符号整数，支持前导零和透明模式。
  - 参数：
    - x, y：数字起始坐标。
    - num：要显示的数字。
    - len：显示长度（位数）。
    - size：字体大小（12/16）。
    - mode：bit7=1 前导零补0，bit0=1 透明模式。
    - color：字体颜色。
    - bg_color：背景色。
  - 示例：
    ```c
    LCD_ShowxNum(10, 70, 42, 4, 16, 0x81, GREEN, BLACK); // 前导零+透明
    ```

## 5. 字体与字模

- `extern const unsigned char asc2_1206[95][12];`
- `extern const unsigned char asc2_1608[95][16];`
  - 常用 ASCII 字模，支持 12x6 和 16x8 两种点阵。
  - 用于 LCD_ShowChar/LCD_ShowString。

## 6. 性能与移植说明

- 所有批量绘制、填充、图片显示均采用大缓冲区分批 SPI 传输，极大提升刷新速度。
- 建议 SPI 速率设置 20MHz 以上，硬件 SPI 推荐 DMA 支持。
- 适配 RT-Thread SPI 设备框架，移植到其他平台需实现 SPI 发送、GPIO 控制等底层接口。
- 支持 LVGL、RT-Thread GUI 等主流嵌入式 GUI 框架。

## 7. 典型用法示例

```c
// 初始化（通常自动完成）
spi_lcd_init();

// 清屏
LCD_Clear(BLACK);

// 显示字符串
LCD_ShowString(10, 10, 100, 20, 16, (u8*)"Hello LCD", RED, WHITE);

// 显示图片
LCD_ShowImage(0, 0, 240, 240, img_data);

// 区域填充
LCD_Fill(20, 20, 100, 100, BLUE);

// 画线
LCD_DrawLine(0, 0, 100, 100, GREEN);
```

## 8. 注意事项

- 坐标参数超出屏幕范围时，部分函数会自动裁剪或忽略。
- 字符显示、图片显示等需保证数据指针有效。
- SPI 速率过低会影响刷新速度，建议使用硬件 SPI。
- 若需自定义字库，可参考 font.h/font.c 实现。
- 方向切换后，lcddev.width/height 会自动调整。

---

如需更详细的底层接口说明，请参考 `LCD_ST7789.c` 源码注释。