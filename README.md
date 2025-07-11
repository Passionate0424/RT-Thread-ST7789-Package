# RT-Thread LCD_ST7789 软件包

## 简介

本软件包基于ST7789驱动IC，适配RT-Thread SPI设备框架，支持240x320分辨率LCD屏，提供丰富的绘图与文本显示API，适合快速集成到RT-Thread项目中。

## 主要特性

- 支持横屏/竖屏切换
- 基本绘图（点、线、矩形、圆、区域填充）
- 文本显示（支持多种字体、颜色、背景色）
- 图片显示（RGB565格式）
- SPI硬件加速，支持DMA批量传输
- 适配RT-Thread设备模型，易于移植

## 目录结构

```
st7789v/
  ├── inc/
  │   ├── LCD_ST7789.h   # 主头文件，API声明
  │   └── font.h         # 字模数据
  ├── src/
  │   └── LCD_ST7789.c   # 主驱动实现
  └── examples/          # 示例代码
```

## API文档

详见 [API_DOC.md](./API_DOC.md) 或 inc/LCD_ST7789.h。

## 使用说明

1. 将st7789v文件夹拷贝到工程目录
2. 在SConscript或Keil工程中添加src和inc路径
3. 在board.c或main.c中调用`spi_lcd_init()`初始化LCD
4. 使用API进行绘图和显示

## 分辨率与显示方向配置说明

### 修改分辨率

1. 打开 `st7789v/inc/LCD_ST7789.h` 头文件。
2. 找到如下宏定义：
   ```c
   #define LCD_W 240  // 屏幕宽度(像素)
   #define LCD_H 320  // 屏幕高度(像素)
   ```
3. 根据实际屏幕参数修改 `LCD_W` 和 `LCD_H` 的数值。
4. 保存文件并重新编译工程。

### 修改默认显示方向

1. 在 `st7789v/inc/LCD_ST7789.h` 中，默认方向宏如下：
   ```c
   #define PORTRAIT  U2D_R2L  // 竖屏方向
   #define LANDSCAPE L2R_U2D  // 横屏方向
   #define Landscape 1        // 1:横屏 0:竖屏
   ```
2. 在应用初始化时（如 `main.c` 或 `board.c`），调用如下API切换方向：
   ```c
   LCD_SetPortrait();   // 设置为竖屏
   LCD_SetLandscape();  // 设置为横屏
   ```
   或直接调用：
   ```c
   LCD_Display_Dir(0); // 0=竖屏, 1=横屏
   ```
3. 若需开机自动切换方向，可在 `spi_lcd_init()` 后立即调用上述API。

### 注意事项
- 分辨率和方向需与实际硬件屏幕参数一致，否则显示内容可能异常。
- 修改分辨率或方向后，建议清空屏幕并重新测试所有显示功能。

## 依赖

- RT-Thread 4.x 及以上
- SPI总线驱动
- rtdevice、rtthread 头文件

## 示例与测试

本软件包自带 `examples/LCD_ST7789_Sample.c` 示例，包含如下测试用例：

- **lcd_test_gradient**：LCD 渐变色显示测试，演示分块渐变填充。
- **lcd_test_char**：字符显示测试，演示多种字体、颜色和ASCII字符表。
- **lcd_test_graphics**：图形绘制测试，演示线条、矩形、圆形等基本图形。

### 运行方法

1. 编译并下载固件后，确保 LCD 已初始化（`spi_lcd_init`）。
2. 在 RT-Thread MSH 命令行输入以下命令运行对应测试：
   - `lcd_test_gradient`  渐变色测试
   - `lcd_test_char`      字符显示测试
   - `lcd_test_graphics`  图形绘制测试

示例代码位于 `st7789v/examples/LCD_ST7789_Sample.c`，可参考或扩展自定义测试。

## 许可证

本软件包遵循 MIT License，详见 LICENSE 文件。

## 贡献

欢迎提交PR和Issue，完善驱动功能和文档。

## 头文件API概览

本驱动头文件（inc/LCD_ST7789.h）主要包含如下内容：

- **LCD参数与硬件宏定义**：屏幕分辨率、引脚定义、SPI总线名、引脚控制宏等，便于移植和硬件适配。
- **类型与方向宏**：常用u8/u16/u32类型定义，支持8种扫描方向，横竖屏切换宏。
- **颜色定义**：常用RGB565颜色宏，便于直接调用。
- **LCD设备结构体**：`_lcd_dev`结构体描述屏幕参数，便于多屏或动态配置。
- **方向切换API**：`LCD_SetPortrait`/`LCD_SetLandscape`，一键切换横竖屏。
- **初始化与配置API**：`spi_lcd_init`、`LCD_Init`、`LCD_Display_Dir`、`LCD_Scan_Dir`等，适配RT-Thread自动初始化和手动配置。
- **基本绘图API**：点、线、矩形、圆、区域填充、图片显示等，接口简洁高效。
- **文本显示API**：支持多字体、颜色、背景色的字符/字符串/数字显示。
- **辅助功能API**：如`LCD_Pow`等常用算法。

所有API均基于RT-Thread驱动框架实现，支持DMA批量传输和高效刷新，便于在RT-Thread项目中直接调用。

详细API说明请参考 [API_DOC.md](./API_DOC.md) 或 `inc/LCD_ST7789.h` 内联注释。
