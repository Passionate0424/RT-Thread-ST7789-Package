/*
 * =====================================================================================
 * @file    LCD_ST7789.c
 * @brief   ST7789 LCD驱动实现 - RT-Thread适配版
 * @author  Passionate0424
 * @date    2025-07-11
 * @version 1.0.0
 *
 * 主要功能：
 *   - LCD初始化配置
 *   - 基本绘图功能实现
 *   - 文本显示功能
 *   - 底层硬件接口封装
 *   - 批量传输优化
 *
 * 移植说明：
 *   1. 适配RT-Thread SPI设备框架
 *   2. 使用RT-Thread PIN框架控制GPIO
 * =====================================================================================
 */

#include "LCD_ST7789.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_common.h>
#include <drv_spi.h> // SPI驱动头文件
#include "font.h"    // 字体数据头文件

// ================= 宏定义与全局变量 =================
#define DBG_TAG "lcd.st7789"
#include <rtdbg.h>

#define LCD_DMA_BUFFER_SIZE 1024
#define LCD_BATCH_BUFFER_SIZE 20480 // 批量传输缓冲区大小(字节)
static uint8_t batch_buffer[LCD_BATCH_BUFFER_SIZE];

u8 DFT_SCAN_DIR;
_lcd_dev lcddev;
static struct rt_spi_device *lcd_dev;

/* 使用lcd_rtthread.h中定义的引脚操作宏 */

/* 移植后的SPI接口函数 */
/**
 * @brief 写LCD寄存器命令
 * @param reg 寄存器地址/命令
 *
 * 功能说明：
 * 1. 拉低DC线表示写命令
 * 2. 通过SPI发送命令字节
 * 3. 拉高DC线恢复数据模式
 */
static void LCD_WR_REG(uint8_t reg)
{
    LCD_DC_CLR;
    rt_spi_send(lcd_dev, &reg, 1);
    LCD_DC_SET;
}

/**
 * @brief 写LCD数据(8位)
 * @param data 要写入的数据
 *
 * 功能说明：
 * 1. 保持DC线为高表示写数据
 * 2. 通过SPI发送单字节数据
 */
static void LCD_WR_DATA(uint8_t data)
{
    LCD_DC_SET;
    rt_spi_send(lcd_dev, &data, 1);
}

/**
 * @brief 写LCD数据(16位)
 * @param data 要写入的16位数据
 *
 * 功能说明：
 * 1. 保持DC线为高表示写数据
 * 2. 将16位数据拆分为高低字节
 * 3. 通过SPI发送两个字节
 */
static void LCD_WR_DATA_16BIT(uint16_t data)
{
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    LCD_DC_SET;
    rt_spi_send(lcd_dev, buf, 2);
}

/* 新增GRAM操作函数 */
/**
 * @brief 写GRAM数据
 * @param RGB_Code RGB565格式颜色值
 *
 * 功能说明：
 * 1. 调用16位数据写入函数
 * 2. 用于连续写入GRAM数据
 */
static void LCD_WriteRAM(uint16_t RGB_Code)
{
    LCD_WR_DATA_16BIT(RGB_Code);
}

/**
 * @brief 准备GRAM写入
 *
 * 功能说明：
 * 1. 发送GRAM写入命令
 * 2. 必须在连续写入GRAM数据前调用
 */
static void LCD_WriteRAM_Prepare(void)
{
    LCD_WR_REG(lcddev.wramcmd);
}

/**
 * @brief 写LCD寄存器值
 * @param LCD_Reg 寄存器地址
 * @param LCD_RegValue 要写入的寄存器值
 *
 * 功能说明：
 * 1. 先写寄存器地址
 * 2. 再写寄存器值(16位)
 */
static void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{
    LCD_WR_REG(LCD_Reg);
    LCD_WR_DATA(LCD_RegValue);
}

/* 保留lcd.c中的初始化命令序列 */
static void LCD_INIT_CODE(u8 dir)
{
    // LCD_BLK_CLR;
    LCD_WR_REG(0x11);
    rt_thread_mdelay(120); // Delay 120ms
    //------------------------------display and color format setting--------------------------------//
    LCD_WR_REG(0x36);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x3a);
    LCD_WR_DATA(0x05);
    //--------------------------------ST7789V Frame rate setting----------------------------------//
    LCD_WR_REG(0xb2);
    LCD_WR_DATA(0x0c);
    LCD_WR_DATA(0x0c);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x33);
    LCD_Display_Dir(dir);
    LCD_WR_REG(0xb7);
    LCD_WR_DATA(0x35);
    //---------------------------------ST7789V Power setting--------------------------------------//
    LCD_WR_REG(0xbb);
    LCD_WR_DATA(0x28);
    LCD_WR_REG(0xc0);
    LCD_WR_DATA(0x2c);
    LCD_WR_REG(0xc2);
    LCD_WR_DATA(0x01);
    LCD_WR_REG(0xc3);
    LCD_WR_DATA(0x10);
    LCD_WR_REG(0xc4);
    LCD_WR_DATA(0x20);
    LCD_WR_REG(0xc6);
    LCD_WR_DATA(0x0f);
    LCD_WR_REG(0xd0);
    LCD_WR_DATA(0xa4);
    LCD_WR_DATA(0xa1);
    //--------------------------------ST7789V gamma setting---------------------------------------//
    // 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23
    LCD_WR_REG(0xe0);
    LCD_WR_DATA(0xd0);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x0a);
    LCD_WR_DATA(0x28);
    LCD_WR_DATA(0x32);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x42);
    LCD_WR_DATA(0x06);
    LCD_WR_DATA(0x0e);
    LCD_WR_DATA(0x12);
    LCD_WR_DATA(0x14);
    LCD_WR_DATA(0x17);
    // 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23
    LCD_WR_REG(0xe1);
    LCD_WR_DATA(0xd0);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x0a);
    LCD_WR_DATA(0x28);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x54);
    LCD_WR_DATA(0x47);
    LCD_WR_DATA(0x0e);
    LCD_WR_DATA(0x1c);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x1b);
    LCD_WR_DATA(0x1e);
    LCD_WR_REG(0x29);
}

/**
 * @brief 设置光标位置
 * @param Xpos X坐标
 * @param Ypos Y坐标
 *
 * 功能说明：
 * 1. 设置X方向光标位置
 * 2. 设置Y方向光标位置
 * 3. 使用批量传输提高效率
 */
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{

    // X坐标设置缓冲区
    uint8_t x_buf[4];
    x_buf[0] = Xpos >> 8;
    x_buf[1] = Xpos & 0X00FF;
    x_buf[2] = Xpos >> 8;
    x_buf[3] = Xpos & 0X00FF;

    // Y坐标设置缓冲区
    uint8_t y_buf[4];
    y_buf[0] = Ypos >> 8;
    y_buf[1] = Ypos & 0X000FF;
    y_buf[2] = Ypos >> 8;
    y_buf[3] = Ypos & 0X00FF;

    // 发送X坐标命令和数据
    LCD_WR_REG(lcddev.setxcmd);
    LCD_DC_SET;
    rt_spi_send(lcd_dev, x_buf, 4);

    // 发送Y坐标命令和数据
    LCD_WR_REG(lcddev.setycmd);
    LCD_DC_SET;
    rt_spi_send(lcd_dev, y_buf, 4);
}


/**
 * @brief 设置LCD显示窗口
 * @param sx 起始X坐标
 * @param sy 起始Y坐标
 * @param width 窗口宽度
 * @param height 窗口高度
 *
 * 功能说明：
 * 1. 设置X方向起始和结束地址
 * 2. 设置Y方向起始和结束地址
 * 3. 使用批量传输提高效率
 */
static void LCD_SetWindows(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{

    uint16_t ex = sx + width - 1;  // 结束X坐标
    uint16_t ey = sy + height - 1; // 结束Y坐标

    // X坐标设置缓冲区
    uint8_t x_buf[4];
    x_buf[0] = sx >> 8;     // 起始X高字节
    x_buf[1] = sx & 0X00FF; // 起始X低字节
    x_buf[2] = ex >> 8;     // 结束X高字节
    x_buf[3] = ex & 0X00FF; // 结束X低字节

    // Y坐标设置缓冲区
    uint8_t y_buf[4];
    y_buf[0] = sy >> 8;     // 起始Y高字节
    y_buf[1] = sy & 0X00FF; // 起始Y低字节
    y_buf[2] = ey >> 8;     // 结束Y高字节
    y_buf[3] = ey & 0X00FF; // 结束Y低字节

    // 发送X坐标命令和数据
    LCD_WR_REG(lcddev.setxcmd);
    LCD_DC_SET;
    rt_spi_send(lcd_dev, x_buf, 4);

    // 发送Y坐标命令和数据
    LCD_WR_REG(lcddev.setycmd);
    LCD_DC_SET;
    rt_spi_send(lcd_dev, y_buf, 4);
}

/* 兼容旧接口 */
/**
 * @brief 设置显示窗口(兼容旧接口)
 * @param sx 起始X坐标
 * @param sy 起始Y坐标
 * @param ex 结束X坐标
 * @param ey 结束Y坐标
 *
 * 功能说明：
 * 1. 转换为宽度/高度格式
 * 2. 调用新式窗口设置函数
 */
static void LCD_SetWindow(u16 sx, u16 sy, u16 ex, u16 ey)
{
    LCD_SetWindows(sx, sy, ex - sx + 1, ey - sy + 1);
}

/**
 * @brief 清屏函数
 * @param Color 填充颜色(RGB565)
 *
 * 功能说明：
 * 1. 设置全屏窗口
 * 2. 使用批量传输模式
 * 3. 用于快速清屏
 */
void LCD_Clear(uint16_t Color)
{

    uint32_t index, i;
    uint32_t total = lcddev.width * lcddev.height;
    uint32_t batch_size = LCD_BATCH_BUFFER_SIZE / 2; // 每批次处理的像素数量(2字节/像素)

    // 设置清屏窗口(全屏)
    LCD_SetWindows(0, 0, lcddev.width, lcddev.height);
    LCD_WriteRAM_Prepare(); // 开始写入GRAM

    // 预填充数据缓冲区
    for (i = 0; i < batch_size && i < LCD_BATCH_BUFFER_SIZE / 2; i++)
    {
        batch_buffer[i * 2] = Color >> 8;       // 高字节
        batch_buffer[i * 2 + 1] = Color & 0xFF; // 低字节
    }

    LCD_DC_SET; // 设置为数据模式

    // 分批次发送数据
    for (index = 0; index < total; index += batch_size)
    {
        // 计算当前批次实际发送的像素数
        uint32_t current_batch = (index + batch_size > total) ? (total - index) : batch_size;

        // 发送批量数据
        rt_spi_send(lcd_dev, batch_buffer, current_batch * 2); // 每像素2字节
    }
}

/* 移植图形绘制函数 */
/**
 * @brief 在指定位置画点
 * @param x X坐标
 * @param y Y坐标
 * @param color 点的颜色(RGB565)
 *
 * 功能说明：
 * 1. 设置光标位置
 * 2. 准备GRAM写入
 * 3. 写入颜色数据
 */
void LCD_DrawPoint(u16 x, u16 y, u16 color)
{

    LCD_SetCursor(x, y);
    LCD_WriteRAM_Prepare();
    LCD_WriteRAM(color);
}

/**
 * @brief 批量绘制多个点
 * @param points_x X坐标数组
 * @param points_y Y坐标数组
 * @param point_count 点的数量
 * @param color 点的颜色(RGB565)
 *
 * 功能说明：
 * 1. 根据点的位置对临近点进行分组
 * 2. 按组进行批量绘制
 * 3. 显著提高多点绘制效率
 */
void LCD_DrawPoints(u16 *points_x, u16 *points_y, u16 point_count, u16 color)
{

    if (point_count == 0)
        return;

    // 如果只有一个点，直接调用单点绘制
    if (point_count == 1)
    {
        LCD_DrawPoint(points_x[0], points_y[0], color);
        return;
    }

    // 为优化性能，寻找连续的点进行批量绘制
    u16 i, j, start_idx;
    u16 batch_count = 0;
    u16 batch_x[256]; // 临时存储批量传输的点坐标
    u16 batch_y[256];

    for (i = 0; i < point_count; i++)
    {
        // 跳过超出屏幕范围的点
        if (points_x[i] >= lcddev.width || points_y[i] >= lcddev.height)
            continue;

        // 添加点到批处理队列
        batch_x[batch_count] = points_x[i];
        batch_y[batch_count] = points_y[i];
        batch_count++;

        // 当积累足够多的点或已处理完所有点时，执行批量绘制
        if (batch_count >= 256 || i == point_count - 1)
        {
            // 寻找可以一次性绘制的水平线段
            start_idx = 0;
            while (start_idx < batch_count)
            {
                // 查找连续的水平线段
                u16 run_length = 1;
                for (j = start_idx + 1; j < batch_count; j++)
                {
                    if (batch_y[j] == batch_y[start_idx] &&
                        batch_x[j] == batch_x[j - 1] + 1)
                    {
                        run_length++;
                    }
                    else
                    {
                        break;
                    }
                }

                // 对于长度大于1的水平线段，使用Fill函数
                if (run_length > 1)
                {
                    LCD_Fill(batch_x[start_idx], batch_y[start_idx],
                             batch_x[start_idx + run_length - 1], batch_y[start_idx],
                             color);
                }
                else
                {
                    // 单个点使用普通绘制
                    LCD_DrawPoint(batch_x[start_idx], batch_y[start_idx], color);
                }

                start_idx += run_length;
            }

            batch_count = 0; // 重置批次计数
        }
    }
}

/**
 * @brief 在指定区域填充单色
 * @param sx 起始X坐标
 * @param sy 起始Y坐标
 * @param ex 结束X坐标
 * @param ey 结束Y坐标
 * @param color 要填充的颜色
 *
 * 功能说明：
 * 1. 设置显示窗口
 * 2. 使用批量传输填充颜色
 * 3. 优化性能，减少SPI传输次数
 */
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{

    u16 width = ex - sx + 1;    // 填充宽度
    u16 height = ey - sy + 1;   // 填充高度
    u32 total = width * height; // 总像素数
    u32 batch_size, i;

    // 设置填充窗口
    LCD_SetWindows(sx, sy, width, height);
    LCD_WriteRAM_Prepare(); // 准备写入GRAM
    LCD_DC_SET;             // 设置为数据模式

    // 计算单次发送的像素数量
    batch_size = LCD_BATCH_BUFFER_SIZE / 2; // 每个像素2字节

    // 预填充数据缓冲区
    for (i = 0; i < batch_size && i < LCD_BATCH_BUFFER_SIZE / 2; i++)
    {
        batch_buffer[i * 2] = color >> 8;       // 高字节
        batch_buffer[i * 2 + 1] = color & 0xFF; // 低字节
    }

    // 分批次发送数据
    for (i = 0; i < total; i += batch_size)
    {
        // 计算本批次要传输的实际像素数
        u32 current_batch = (i + batch_size > total) ? (total - i) : batch_size;

        // 发送批量数据
        rt_spi_send(lcd_dev, batch_buffer, current_batch * 2); // 每像素2字节
    }
}

/* 字体显示函数 */
/**
 * @brief 显示单个字符
 * @param x X坐标
 * @param y Y坐标
 * @param chr 要显示的字符
 * @param size 字体大小(12/16)
 * @param mode 显示模式(0-覆盖模式,1-叠加模式)
 * @param color 字符颜色
 * @param bg_color 背景颜色
 *
 * 功能说明：
 * 1. 计算字符在字库中的偏移
 * 2. 获取字模数据
 * 3. 根据不同模式高效绘制
 */
void LCD_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color, uint16_t bg_color)
{

    uint8_t temp;
    uint16_t x0 = x;
    uint16_t y0 = y;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); // 计算字模数据大小
    uint8_t *pfont = 0;
    uint16_t char_width = size / 2;                // 字符宽度
    uint16_t char_height = size;                   // 字符高度
    uint16_t conv[char_width][char_height];        // 临时存储字符数据
    memset(conv, 0, sizeof(conv));                 // 清空临时存储区
    memset(batch_buffer, 0, sizeof(batch_buffer)); // 清空临时存储区

    // 检查边界
    if (x + char_width > lcddev.width || y + char_height > lcddev.height)
    {

        return;
    }

    // 字符偏移量计算
    chr = chr - ' ';

    // 获取字体数据指针
    switch (size)
    {
    case 12:
        pfont = (uint8_t *)asc2_1206[chr];
        break;
    case 16:
        pfont = (uint8_t *)asc2_1608[chr];
        break;
    default:
        LOG_E("Unsupported font size: %d", size);

        return;
    }

    for (int t = 0; t < csize; t++)
    {
        temp = pfont[t]; /* 获取字符的点阵数据 */

        for (int t1 = 0; t1 < 8; t1++) /* 一个字节8个点 */
        {
            if (temp & 0x80) /* 有效点,需要显示 */
            {
                conv[x - x0][y - y0] = color;
            }
            else if (mode == 0) /* 无效点,不显示 */
            {
                conv[x - x0][y - y0] = bg_color;
            }

            temp <<= 1; /* 移位, 以便获取下一个位的状态 */
            y++;

            if (y >= lcddev.height)
                return; /* 超区域了 */

            if ((y - y0) == size) /* 显示完一列了? */
            {
                y = y0; /* y坐标复位 */
                x++;    /* x坐标递增 */

                if (x >= lcddev.width)
                {
                    return; /* x坐标超区域了 */
                }

                break;
            }
        }
    }

    // 非透明模式，可以直接设置窗口进行批量绘制
    if (mode == 0)
    {
        // 设置显示窗口
        LCD_SetWindows(x0, y0, char_width, char_height);
        LCD_WriteRAM_Prepare();
        LCD_DC_SET;

        uint16_t xs = 0, ys = 0; // 字符内部坐标
        uint32_t pixel_idx = 0;  // 批处理缓冲区索引

        for (int i = 0; i < char_height; i++)
        {
            for (int j = 0; j < char_width; j++)
            {
                // 将颜色数据放入缓冲区
                batch_buffer[pixel_idx++] = conv[j][i] >> 8;
                batch_buffer[pixel_idx++] = conv[j][i] & 0xFF;

                // 如果缓冲区满了，发送数据
                if (pixel_idx >= LCD_BATCH_BUFFER_SIZE)
                {
                    rt_spi_send(lcd_dev, batch_buffer, pixel_idx);
                    pixel_idx = 0;
                }
            }
        }

        // 发送剩余数据
        if (pixel_idx > 0)
        {
            rt_spi_send(lcd_dev, batch_buffer, pixel_idx);
        }
    }
    else // 透明模式
    {
        for (uint8_t t = 0; t < csize; t++)
        {
            temp = pfont[t];
            uint16_t pos_x = x + t / (char_height / 8);
            uint16_t pos_y = y + t % (char_height / 8) * 8;

            // 一个字节8个点
            for (uint8_t i = 0; i < 8; i++)
            {
                // 只绘制字体数据为1的点
                if (temp & 0x80)
                    LCD_DrawPoint(pos_x, pos_y + i, color);
                temp <<= 1;

                // 检查是否到达字符高度
                if (pos_y + i >= y + char_height - 1)
                    break;
            }
        }
    }
}

/**
 * @brief 在LCD上显示字符串
 * @param x 起始X坐标(像素)
 * @param y 起始Y坐标(像素)
 * @param width 显示区域宽度(像素)
 * @param height 显示区域高度(像素)
 * @param size 字体大小(12/16/24等)
 * @param p 要显示的字符串指针
 * @param color 文字颜色(RGB565格式)
 * @param bg_color 背景颜色(RGB565格式)
 *
 * 功能说明：
 * 1. 在指定区域内显示字符串
 * 2. 自动换行处理(当超出width时换到下一行)
 * 3. 支持ASCII字符(32~126)
 * 4. 超出显示区域的内容会被裁剪
 * 5. 可自定义文字和背景颜色
 *
 * 注意：
 * - 使用前需先初始化LCD
 * - 确保坐标和区域大小在屏幕范围内
 * - 背景色参数为新增功能，旧代码需要更新
 */
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p, u16 color, uint16_t bg_color)
{

    u8 x0 = x;
    width += x;
    height += y;
    while ((*p <= '~') && (*p >= ' ')) // 判断是不是非法字符!
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }
        if (y >= height)
            break; // 退出
        LCD_ShowChar(x, y, *p, size, 0, color, bg_color);
        x += size / 2;
        p++;
    }
}

/**
 * @brief 显示数字
 * @param x X坐标
 * @param y Y坐标
 * @param num 要显示的数字
 * @param len 数字长度
 * @param size 字体大小
 * @param color 文字颜色
 * @param bg_color 背景颜色
 *
 * 功能说明：
 * 1. 支持固定长度显示
 * 2. 自动处理前导零
 * 3. 可自定义文字和背景颜色
 */
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size, u16 color, uint16_t bg_color)
{

    u8 t, temp;
    u8 enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / LCD_Pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                LCD_ShowChar(x + (size / 2) * t, y, ' ', size, 0, color, bg_color);
                continue;
            }
            else
                enshow = 1;
        }
        LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, 0, color, bg_color);
    }
}

/**
 * @brief 显示数字(增强版)
 * @param x X坐标
 * @param y Y坐标
 * @param num 要显示的数字
 * @param len 数字长度
 * @param size 字体大小
 * @param mode 显示模式
 * @param color 颜色
 *
 * 功能说明：
 * 1. 支持多种显示模式
 * 2. 可控制前导零显示
 * 3. 调用字符显示函数实现
 */
void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode, u16 color, uint16_t bg_color)
{

    u8 t, temp;
    u8 enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / LCD_Pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                if (mode & 0X80)
                    LCD_ShowChar(x + (size / 2) * t, y, '0', size, mode & 0X01, color, bg_color);
                else
                    LCD_ShowChar(x + (size / 2) * t, y, ' ', size, mode & 0X01, color, BACK_COLOR);
                continue;
            }
            else
                enshow = 1;
        }
        LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, mode & 0X01, color, BACK_COLOR);
    }
}

/**
 * @brief 计算m的n次方
 * @param m 底数
 * @param n 指数
 * @return m^n结果
 *
 * 功能说明：
 * 1. 用于数字显示计算
 * 2. 简单循环实现
 */
u32 LCD_Pow(u8 m, u8 n)
{
    u32 result = 1;
    while (n--)
        result *= m;
    return result;
}

/**
 * @brief 显示图片
 * @param x X坐标
 * @param y Y坐标
 * @param width 图片宽度
 * @param height 图片高度
 * @param p 图片数据指针(RGB565格式)
 *
 * 功能说明：
 * 1. 设置显示窗口
 * 2. 批量传输像素数据
 * 3. 支持RGB565格式图片
 * 4. 使用优化的批量传输方式
 */
void LCD_ShowImage(u16 x, u16 y, u16 width, u16 height, const u16 *p)
{
    u32 i, j;
    u32 total = width * height;
    u32 batch_size = LCD_BATCH_BUFFER_SIZE / 2; // 每批次处理的像素数量(2字节/像素)
    u32 current_batch;

    // 设置窗口
    LCD_SetWindows(x, y, width, height);
    LCD_WriteRAM_Prepare(); // 开始写入GRAM
    LCD_DC_SET;             // 设置为数据模式

    // 分批次发送数据
    for (i = 0; i < total; i += batch_size)
    {
        // 计算本批次要传输的实际像素数
        current_batch = (i + batch_size > total) ? (total - i) : batch_size;

        // 将当前批次的RGB565数据转换为字节流
        for (j = 0; j < current_batch; j++)
        {
            u16 color = p[i + j];
            batch_buffer[j * 2] = color >> 8;       // 高字节
            batch_buffer[j * 2 + 1] = color & 0xFF; // 低字节
        }

        // 发送批量数据
        rt_spi_send(lcd_dev, batch_buffer, current_batch * 2); // 每像素2字节
    }
}

/**
 * @brief 画线函数
 * @param x1 起点X坐标
 * @param y1 起点Y坐标
 * @param x2 终点X坐标
 * @param y2 终点Y坐标
 * @param color 线条颜色
 *
 * 功能说明：
 * 1. 使用Bresenham算法
 * 2. 支持任意方向直线
 * 3. 收集所有点后一次批量绘制
 */
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{

    int xerr = 0, yerr = 0, distance;
    int incx, incy, row, col;
    u16 points_x[512]; // 点坐标缓冲区
    u16 points_y[512];
    u16 point_count = 0;

    // 特殊情况处理 - 水平或垂直线可直接使用Fill函数
    if (x1 == x2)
    { // 垂直线
        // 确保y1 <= y2
        if (y1 > y2)
        {
            u16 temp = y1;
            y1 = y2;
            y2 = temp;
        }
        LCD_Fill(x1, y1, x1, y2, color);

        return;
    }

    if (y1 == y2)
    { // 水平线
        // 确保x1 <= x2
        if (x1 > x2)
        {
            u16 temp = x1;
            x1 = x2;
            x2 = temp;
        }
        LCD_Fill(x1, y1, x2, y1, color);

        return;
    }

    // Bresenham算法参数计算
    int delta_x = x2 - x1; // 计算坐标增量
    int delta_y = y2 - y1;

    // 设置单步方向
    incx = (delta_x > 0) ? 1 : -1;
    incy = (delta_y > 0) ? 1 : -1;

    // 选取基本增量坐标轴
    distance = (abs(delta_x) > abs(delta_y)) ? abs(delta_x) : abs(delta_y);

    // 收集线段上的所有点坐标
    for (u16 i = 0; i <= distance + 1; i++)
    {
        // 添加点到缓冲区
        if (point_count < 512)
        {
            points_x[point_count] = x1;
            points_y[point_count] = y1;
            point_count++;
        }

        // Bresenham算法计算下一个点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            x1 += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            y1 += incy;
        }
    }

    // 分析点的特征，寻找优化空间
    // 1. 检测连续的水平线段
    u16 start_x = 0;
    u16 current_y = points_y[0];
    u16 horizontal_len = 1;
    u16 processed = 0;

    // 创建渲染区域
    // 计算线的边界框
    u16 min_x = points_x[0], max_x = points_x[0];
    u16 min_y = points_y[0], max_y = points_y[0];
    for (u16 i = 1; i < point_count; i++)
    {
        if (points_x[i] < min_x)
            min_x = points_x[i];
        if (points_x[i] > max_x)
            max_x = points_x[i];
        if (points_y[i] < min_y)
            min_y = points_y[i];
        if (points_y[i] > max_y)
            max_y = points_y[i];
    }

    // 如果线段较短（少于30个点），或者点极为分散，直接使用DrawPoints批量绘制
    if (point_count < 30 || (max_x - min_x > 100) || (max_y - min_y > 100))
    {
        LCD_DrawPoints(points_x, points_y, point_count, color);

        return;
    }

    // 对于更长的线段，分析可能的连续区域
    for (u16 i = 1; i < point_count; i++)
    {
        // 检测水平线段
        if (points_y[i] == current_y && points_x[i] == points_x[i - 1] + 1)
        {
            horizontal_len++;
        }
        else
        {
            // 水平线段结束
            if (horizontal_len > 3)
            {
                // 使用Fill绘制水平线段
                LCD_Fill(points_x[start_x], current_y,
                         points_x[start_x + horizontal_len - 1], current_y, color);
                processed += horizontal_len;
            }
            else if (horizontal_len > 0)
            {
                // 短线段，暂存
                for (u16 j = 0; j < horizontal_len; j++)
                {
                    LCD_DrawPoint(points_x[start_x + j], current_y, color);
                }
                processed += horizontal_len;
            }

            // 开始新的线段
            start_x = i;
            current_y = points_y[i];
            horizontal_len = 1;
        }
    }

    // 处理最后一个线段
    if (horizontal_len > 3)
    {
        LCD_Fill(points_x[start_x], current_y,
                 points_x[start_x + horizontal_len - 1], current_y, color);
        processed += horizontal_len;
    }
    else if (horizontal_len > 0)
    {
        for (u16 j = 0; j < horizontal_len; j++)
        {
            LCD_DrawPoint(points_x[start_x + j], current_y, color);
        }
        processed += horizontal_len;
    }

    // 绘制未处理的点（如果有）
    if (processed < point_count)
    {
        // 创建未处理点的新缓冲区
        u16 remaining = point_count - processed;
        u16 remaining_x[512];
        u16 remaining_y[512];
        u16 count = 0;

        // 识别未处理点
        for (u16 i = 0; i < point_count; i++)
        {
            // 检查是否是已处理点（设置为特定值）
            if (points_x[i] != 0xFFFF)
            {
                remaining_x[count] = points_x[i];
                remaining_y[count] = points_y[i];
                count++;
            }
        }

        // 批量绘制剩余点
        if (count > 0)
        {
            LCD_DrawPoints(remaining_x, remaining_y, count, color);
        }
    }
}

/**
 * @brief 画矩形
 * @param x1 左上角X坐标
 * @param y1 左上角Y坐标
 * @param x2 右下角X坐标
 * @param y2 右下角Y坐标
 * @param color 边框颜色
 *
 * 功能说明：
 * 1. 调用画线函数实现
 * 2. 绘制四条边组成矩形
 */
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{

    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}

/**
 * @brief 画圆
 * @param x0 圆心X坐标
 * @param y0 圆心Y坐标
 * @param r 半径
 * @param color 圆颜色
 *
 * 功能说明：
 * 1. 使用Bresenham算法
 * 2. 对称绘制8个点
 * 3. 使用批量绘制函数一次性传输
 */
void Draw_Circle(u16 x0, u16 y0, u8 r, u16 color)
{

    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1); // 判断下个点位置的标志

    // 用于批量绘制的点坐标缓冲区
    u16 points_x[512];
    u16 points_y[512];
    u16 point_count = 0;

    while (a <= b)
    {
        // 存储8个对称点到缓冲区
        points_x[point_count] = x0 + a;
        points_y[point_count++] = y0 - b; // 5
        points_x[point_count] = x0 + b;
        points_y[point_count++] = y0 - a; // 0
        points_x[point_count] = x0 + b;
        points_y[point_count++] = y0 + a; // 4
        points_x[point_count] = x0 + a;
        points_y[point_count++] = y0 + b; // 6
        points_x[point_count] = x0 - a;
        points_y[point_count++] = y0 + b; // 1
        points_x[point_count] = x0 - b;
        points_y[point_count++] = y0 + a; // 7
        points_x[point_count] = x0 - a;
        points_y[point_count++] = y0 - b; // 2
        points_x[point_count] = x0 - b;
        points_y[point_count++] = y0 - a; // 3

        // 缓冲区即将满时绘制
        if (point_count >= 500)
        {
            // 使用批量绘制函数一次性处理所有点
            LCD_DrawPoints(points_x, points_y, point_count, color);
            point_count = 0; // 重置计数器
        }

        a++;
        // 使用Bresenham算法画圆
        if (di < 0)
            di += 4 * a + 6;
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }

    // 绘制剩余的点
    if (point_count > 0)
    {
        // 使用批量绘制函数一次性处理所有点
        LCD_DrawPoints(points_x, points_y, point_count, color);
    }
}

/**
 * @brief 设置LCD扫描方向
 * @param dir 方向参数(0~7)
 *
 * 功能说明：
 * 1. 配置扫描方向寄存器
 * 2. 影响显示内容方向
 * 3. 支持8种方向设置
 */
void LCD_Scan_Dir(u8 dir)
{

    u16 regval = 0;
    u8 dirreg = 0;
    switch (dir)
    {
    case 0:
        dir = 6;
        break;
    case 1:
        dir = 7;
        break;
    case 2:
        dir = 4;
        break;
    case 3:
        dir = 5;
        break;
    case 4:
        dir = 1;
        break;
    case 5:
        dir = 0;
        break;
    case 6:
        dir = 3;
        break;
    case 7:
        dir = 2;
        break;
    }
    rt_kprintf("dir:%d\n", dir);
    switch (dir)
    {
    case L2R_U2D: // 从左到右,从上到下
        regval = (0 << 7) | (0 << 6) | (0 << 5);
        break;
    case L2R_D2U: // 从左到右,从下到上
        regval = (1 << 7) | (0 << 6) | (0 << 5);
        break;
    case R2L_U2D: // 从右到左,从上到下
        regval = (0 << 7) | (1 << 6) | (0 << 5);
        break;
    case R2L_D2U: // 从右到左,从下到上
        regval = (1 << 7) | (1 << 6) | (0 << 5);
        break;
    case U2D_L2R: // 从上到下,从左到右
        regval = (0 << 7) | (0 << 6) | (1 << 5);
        break;
    case U2D_R2L: // 从上到下,从右到左
        regval = (0 << 7) | (1 << 6) | (1 << 5);
        break;
    case D2U_L2R: // 从下到上,从左到右
        regval = (1 << 7) | (0 << 6) | (1 << 5);

        break;
    case D2U_R2L: // 从下到上,从右到左
        regval = (1 << 7) | (1 << 6) | (1 << 5);
        break;
    }
    dirreg = 0X36;
    regval |= 0x00; // 0x08 0x00  红蓝反色可以通过这里修改
    rt_kprintf("regval:%x\n", regval);
    LCD_WriteReg(dirreg, regval);

    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(0);
    LCD_WR_DATA(0);
    LCD_WR_DATA((lcddev.width - 1) >> 8);
    LCD_WR_DATA((lcddev.width - 1) & 0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(0);
    LCD_WR_DATA(0);
    LCD_WR_DATA((lcddev.height - 1) >> 8);
    LCD_WR_DATA((lcddev.height - 1) & 0XFF);
}

/**
 * @brief 设置LCD显示方向
 * @param dir 方向(0-竖屏,1-横屏)
 *
 * 功能说明：
 * 1. 配置显示参数
 * 2. 更新宽高值
 * 3. 设置默认扫描方向
 */
void LCD_Display_Dir(u8 dir)
{
    if (dir == 0)
    {
        lcddev.dir = 0; // 竖屏
        lcddev.width = LCD_W;
        lcddev.height = LCD_H;

        lcddev.wramcmd = 0X2C;
        lcddev.setxcmd = 0X2A;
        lcddev.setycmd = 0X2B;
        DFT_SCAN_DIR = U2D_R2L; // 竖显-设定显示方向
    }
    else // 横屏
    {
        lcddev.dir = 1;
        lcddev.width = LCD_H;
        lcddev.height = LCD_W;

        lcddev.wramcmd = 0X2C;
        lcddev.setxcmd = 0X2A;
        lcddev.setycmd = 0X2B;
        DFT_SCAN_DIR = L2R_U2D; // 横显-设定显示方向
    }
    LCD_Scan_Dir(DFT_SCAN_DIR); // 默认扫描方向
}

/**
 * @brief 颜色块填充
 * @param sx 起始X坐标
 * @param sy 起始Y坐标
 * @param ex 结束X坐标
 * @param ey 结束Y坐标
 * @param color 颜色数据指针(RGB565格式)
 *
 * 功能说明：
 * 1. 设置显示窗口
 * 2. 使用批量传输优化
 * 3. 支持任意矩形区域填充
 * 4. 显著提升显示效率
 */
void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color)
{

    u16 width = ex - sx + 1;    // 填充宽度
    u16 height = ey - sy + 1;   // 填充高度
    u32 total = width * height; // 总像素数
    u32 batch_size, i, j, k = 0;
    u32 current_batch;

    // 设置填充窗口
    LCD_SetWindows(sx, sy, width, height);
    LCD_WriteRAM_Prepare(); // 准备写入GRAM
    LCD_DC_SET;             // 设置为数据模式

    // 计算每次批量传输的像素数量
    batch_size = LCD_BATCH_BUFFER_SIZE / 2; // 每个像素2字节

    // 分批次发送数据
    for (i = 0; i < total; i += batch_size)
    {
        // 计算本批次要传输的实际像素数
        current_batch = (i + batch_size > total) ? (total - i) : batch_size;

        // 将当前批次的RGB565数据转换为字节流
        for (j = 0; j < current_batch; j++)
        {
            u16 clr = color[k++];                 // 获取当前像素颜色
            batch_buffer[j * 2] = clr >> 8;       // 高字节
            batch_buffer[j * 2 + 1] = clr & 0xFF; // 低字节
        }

        // 发送批量数据
        rt_spi_send(lcd_dev, batch_buffer, current_batch * 2); // 每像素2字节
    }
}

/**
 * @brief 在指定区域内填充指定数据(LVGL优化版)
 * @param x 左上角起始X坐标
 * @param y 左上角起始Y坐标
 * @param x2 右下角结束X坐标
 * @param y2 右下角结束Y坐标
 * @param pData 数据指针(RGB565格式)
 * 功能说明：
 * 1. 适用于LVGL移植的disp_flush函数
 * 2. 使用一次性SPI传输提高效率
 * 3. 支持16位RGB565格式数据(高位在前)
 * 4. 针对大数据量优化，显著提升刷新速度
 */
void LCD_DispFlush(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint16_t *pData)
{
    // 计算宽度和高度
    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;

    // 总像素数
    uint32_t total_size = width * height;
    uint32_t batch_size = LCD_BATCH_BUFFER_SIZE / 2; // 每批次处理的像素数量(2字节/像素)
    uint32_t bytes_to_send, current_batch;
    uint32_t i = 0;

    // 设置窗口
    LCD_SetWindows(x1, y1, width, height);
    LCD_WriteRAM_Prepare(); // 准备写入GRAM
    LCD_DC_SET;             // 设置为数据模式

    // 分批次发送数据(处理大于缓冲区大小的数据)
    while (i < total_size)
    {
        // 计算本批次要传输的实际像素数
        current_batch = (i + batch_size > total_size) ? (total_size - i) : batch_size;
        bytes_to_send = current_batch * 2; // 每像素2字节

        // 填充发送缓冲区
        for (uint32_t j = 0; j < current_batch; j++)
        {
            uint16_t color = pData[i + j];
            batch_buffer[j * 2] = color >> 8;       // 高字节
            batch_buffer[j * 2 + 1] = color & 0xFF; // 低字节
        }

        // 一次性发送数据块
        rt_spi_send(lcd_dev, batch_buffer, bytes_to_send);

        // 更新发送位置
        i += current_batch;
    }
}

/* 快速方向切换函数实现 */
void LCD_SetPortrait(void)
{
    LCD_Scan_Dir(PORTRAIT);
    lcddev.dir = 0; // 竖屏
    lcddev.width = LCD_W;
    lcddev.height = LCD_H;
}

void LCD_SetLandscape(void)
{
    LCD_Scan_Dir(LANDSCAPE);
    lcddev.dir = 1; // 横屏
    lcddev.width = LCD_H;
    lcddev.height = LCD_W;
}

/**
 * @brief 绘制测试图案
 *
 * 功能说明：
 * 1. 绘制9宫格色块
 * 2. 每种颜色不同亮度
 * 3. 用于LCD功能测试
 */
static void LCD_DrawTestPattern(void)
{
    LOG_D("Starting optimized test pattern...");

    /* 确保背光开启 */
#if (BACKLIGHT_ACTIVE_LEVEL == 1)
    LCD_BLK_SET;
#else
    LCD_BLK_CLR;
#endif

    /* 先清屏为白色 */
    LCD_Clear(WHITE);
    rt_thread_mdelay(100);

    /* 计算区域尺寸 */
    uint16_t w = lcddev.width / 3;
    uint16_t h = lcddev.height / 3;

    /* 绘制9个色块 */
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            uint16_t color;
            /* 确定基础颜色 */
            switch (col)
            {
            case 0:
                color = RED;
                break;
            case 1:
                color = GREEN;
                break;
            case 2:
                color = BLUE;
                break;
            }

            /* 调整亮度 */
            switch (row)
            {
            case 0:
                color |= 0xFFFF;
                break; // 全亮
            case 1:
                color &= 0xFFE0;
                break; // 半亮
            case 2:
                color &= 0xF800;
                break; // 暗
            }

            /* 计算区域坐标 */
            uint16_t x1 = col * w;
            uint16_t y1 = row * h;
            uint16_t x2 = (col == 2) ? lcddev.width - 1 : (col + 1) * w - 1;
            uint16_t y2 = (row == 2) ? lcddev.height - 1 : (row + 1) * h - 1;

            LOG_D("Drawing block %d-%d: (%d,%d)-(%d,%d) color=0x%04X",
                  row, col, x1, y1, x2, y2, color);

            /* 填充区域 */
            LCD_Fill(x1, y1, x2, y2, color);

            /* 添加短暂延时防止SPI过载 */
            rt_thread_mdelay(10);
        }
    }
    LOG_D("Test pattern complete");
}

/* RT-Thread设备初始化 */
static void lcd_pin_init(void)
{
    rt_pin_mode(LCD_DC_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_RES_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_BLK_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_CS_PIN, PIN_MODE_OUTPUT);
}

int spi_lcd_init(void)
{
    rt_err_t res = RT_EOK;

    /* 初始化引脚 */
    lcd_pin_init();

    /* 附加SPI设备 */
    rt_hw_spi_device_attach(LCD_SPI_BUS, "spi_lcd", LCD_CS_PIN);

    /* 查找SPI设备 */
    lcd_dev = (struct rt_spi_device *)rt_device_find("spi_lcd");
    if (lcd_dev != RT_NULL)
    {
        /* 配置SPI参数 */
        struct rt_spi_configuration spi_config;
        spi_config.data_width = 8;
        spi_config.max_hz = 25 * 1000 * 1000; /* 25MHz */
        spi_config.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
        rt_spi_configure(lcd_dev, &spi_config);
    }
    else
    {
        res = -RT_ERROR;
        LOG_E("SPI device not found!");
    }
    rt_thread_mdelay(25);
    LCD_RES_CLR;
    rt_thread_mdelay(25);
    LCD_RES_SET;
    rt_thread_mdelay(50);

    /* 执行LCD初始化序列 */
    LCD_INIT_CODE(Landscape);
    rt_thread_mdelay(10); // 确保方向设置完成
}
INIT_COMPONENT_EXPORT(spi_lcd_init);
