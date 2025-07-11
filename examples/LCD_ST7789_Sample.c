/*
 * =====================================================================================
 * @file    LCD_ST7789_Sample.c
 * @brief   ST7789 LCD驱动示例 - RT-Thread下的测试用例
 * @author  （请补充作者）
 * @date    2025-07-11
 * @version 1.0
 *
 * 主要内容：
 *   - LCD渐变色显示测试
 *   - 字符显示测试
 *   - 图形绘制测试
 *   - 通过MSH命令调用
 *
 * 使用说明：
 *   1. 需先初始化LCD（spi_lcd_init）
 *   2. 在MSH下输入 lcd_test_gradient / lcd_test_char / lcd_test_graphics 运行对应测试
 * =====================================================================================
 */

#include "LCD_ST7789.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_common.h>
#include <drv_spi.h> /* 添加SPI驱动头文件 */
#include "font.h"    /* 包含字体数据头文件 */

/* 颜色数组，用于图形测试 */
static uint16_t color_array[] = {
    WHITE, BLACK, BLUE, RED,
    GREEN, YELLOW};

/**
 * @brief LCD显示渐变色测试
 * @return 0-成功 -1-失败
 *
 * 功能说明：
 * 1. 分块绘制渐变色背景
 * 2. 可通过MSH命令调用：lcd_test_gradient
 */
int lcd_test_gradient(int argc, char **argv)
{
    if (argc > 1)
    {
        rt_kprintf("Usage: lcd_test_gradient\n");
        return -1;
    }
    static rt_device_t lcd_dev = RT_NULL;
    lcd_dev = rt_device_find("spi_lcd");
    if (!lcd_dev)
    {
        rt_kprintf("LCD not initialized, run 'rt_hw_lcd_init' first\n");
        return -1;
    }

    LCD_Clear(BLACK);

    /* 测试3: 绘制渐变 - 优化为分块绘制 */
    LCD_Clear(BLACK);
    rt_kprintf("Test3: Drawing optimized gradient...\n");

    /* 强制设置扫描方向为从左到右，从上到下 */
    rt_thread_mdelay(10); // 确保方向设置完成

#define GRADIENT_BLOCK_SIZE 16 // 16x16像素块

    for (uint16_t block_y = 0; block_y < lcddev.height; block_y += GRADIENT_BLOCK_SIZE)
    {
        uint16_t end_y = (block_y + GRADIENT_BLOCK_SIZE > lcddev.height) ? lcddev.height : block_y + GRADIENT_BLOCK_SIZE;

        for (uint16_t block_x = 0; block_x < lcddev.width; block_x += GRADIENT_BLOCK_SIZE)
        {
            uint16_t end_x = (block_x + GRADIENT_BLOCK_SIZE > lcddev.width) ? lcddev.width : block_x + GRADIENT_BLOCK_SIZE;

            // 计算块四角颜色(双线性插值)
            uint16_t x1 = block_x;
            uint16_t y1 = block_y;
            uint16_t x2 = (block_x + GRADIENT_BLOCK_SIZE >= lcddev.width) ? lcddev.width - 1 : block_x + GRADIENT_BLOCK_SIZE;
            uint16_t y2 = (block_y + GRADIENT_BLOCK_SIZE >= lcddev.height) ? lcddev.height - 1 : block_y + GRADIENT_BLOCK_SIZE;

            // 计算四角颜色
            uint16_t c00_r = (x1 * 31) / (lcddev.width - 1);
            uint16_t c00_g = (y1 * 63) / (lcddev.height - 1);
            uint16_t c00_b = 31 - (x1 * 31) / (lcddev.width - 1);
            uint16_t c00 = (c00_r << 11) | (c00_g << 5) | c00_b;

            uint16_t c01_r = (x1 * 31) / (lcddev.width - 1);
            uint16_t c01_g = (y2 * 63) / (lcddev.height - 1);
            uint16_t c01_b = 31 - (x1 * 31) / (lcddev.width - 1);
            uint16_t c01 = (c01_r << 11) | (c01_g << 5) | c01_b;

            uint16_t c10_r = (x2 * 31) / (lcddev.width - 1);
            uint16_t c10_g = (y1 * 63) / (lcddev.height - 1);
            uint16_t c10_b = 31 - (x2 * 31) / (lcddev.width - 1);
            uint16_t c10 = (c10_r << 11) | (c10_g << 5) | c10_b;

            uint16_t c11_r = (x2 * 31) / (lcddev.width - 1);
            uint16_t c11_g = (y2 * 63) / (lcddev.height - 1);
            uint16_t c11_b = 31 - (x2 * 31) / (lcddev.width - 1);
            uint16_t c11 = (c11_r << 11) | (c11_g << 5) | c11_b;

            // 填充块(逐行绘制渐变)
            for (uint16_t y = y1; y <= y2; y++)
            {
                for (uint16_t x = x1; x <= x2; x++)
                {
                    // 双线性插值计算颜色
                    float tx = (float)(x - x1) / (x2 - x1);
                    float ty = (float)(y - y1) / (y2 - y1);

                    uint16_t r = (1 - tx) * (1 - ty) * c00_r + tx * (1 - ty) * c10_r +
                                 (1 - tx) * ty * c01_r + tx * ty * c11_r;
                    uint16_t g = (1 - tx) * (1 - ty) * c00_g + tx * (1 - ty) * c10_g +
                                 (1 - tx) * ty * c01_g + tx * ty * c11_g;
                    uint16_t b = (1 - tx) * (1 - ty) * c00_b + tx * (1 - ty) * c10_b +
                                 (1 - tx) * ty * c01_b + tx * ty * c11_b;
                    uint16_t color = (r << 11) | (g << 5) | b;

                    LCD_DrawPoint(x, y, color);
                }
            }

            // 每5个块打印一次进度
            if ((block_x / GRADIENT_BLOCK_SIZE) % 5 == 0)
            {
                rt_kprintf("Drawing block (%d,%d) color=0x%04X\n",
                           block_x, block_y);
            }
        }

        // 每行块之间添加延时
        rt_thread_mdelay(5);
    }
    rt_thread_mdelay(500);

    return 0;
}
MSH_CMD_EXPORT(lcd_test_gradient, "Test LCD gradient drawing");

/**
 * @brief LCD显示字符测试
 * @return 0-成功 -1-失败
 *
 * 功能说明：
 * 1. 显示不同大小和颜色的字符串
 * 2. 显示ASCII字符表
 * 3. 可通过MSH命令调用：lcd_test_char
 */
int lcd_test_char(int argc, char **argv)
{
    if (argc > 1)
    {
        rt_kprintf("Usage: lcd_test_char\n");
        return -1;
    }
    static rt_device_t lcd_dev = RT_NULL;
    lcd_dev = rt_device_find("spi_lcd");
    if (!lcd_dev)
    {
        rt_kprintf("LCD not initialized, run 'rt_hw_lcd_init' first\n");
        return -1;
    }

    /* 测试4: 显示字符 */
    LCD_Display_Dir(0);
    LCD_Clear(BLACK);
    rt_kprintf("Test4: Drawing characters...\n");


    // 显示ASCII字符表
    for (int i = 0; i < 5; i++)
    {
        char buf[20];
        rt_snprintf(buf, sizeof(buf), "ASCII %d-%d", 32 + i * 16, 47 + i * 16);
        LCD_ShowString(10, 160 + i * 20, 200, 16, 16, (u8 *)buf, YELLOW, BACK_COLOR);
    }
    rt_thread_mdelay(2000);
    return 0;
}
MSH_CMD_EXPORT(lcd_test_char, "Test LCD character display");

/*
 * @brief LCD图形绘制测试
 * @return 0-成功 -1-失败
 *
 * 功能说明：
 * 1. 测试线条、矩形、圆形等图形绘制
 * 2. 可通过MSH命令调用：lcd_test_graphics
 */
int lcd_test_graphics(int argc, char **argv)
{
    if (argc > 1)
    {
        rt_kprintf("Usage: lcd_test_char\n");
        return -1;
    }
    static rt_device_t lcd_dev = RT_NULL;
    lcd_dev = rt_device_find("spi_lcd");
    if (!lcd_dev)
    {
        rt_kprintf("LCD not initialized, run 'rt_hw_lcd_init' first\n");
        return -1;
    }

    /* 测试5: 图形绘制测试 */
    LCD_Clear(BLACK);

    rt_kprintf("\n=== Test5: Graphics drawing ===\n");

    // 绘制线条
    for (int i = 0; i < 5; i++)
    {
        LCD_DrawLine(10, 30 + i * 20, 200, 30 + i * 20, color_array[i]);
    }
    rt_kprintf("Lines drawn\n");
    rt_thread_mdelay(1000);

    // 绘制矩形
    LCD_DrawRectangle(50, 50, 150, 150, RED);
    LCD_DrawRectangle(60, 60, 140, 140, GREEN);
    rt_kprintf("Rectangles drawn\n");
    rt_thread_mdelay(1000);

    // 绘制圆形
    for (int i = 0; i < 3; i++)
    {
        Draw_Circle(100, 100, 30 + i * 10, BLUE);
    }
    rt_kprintf("Circles drawn\n");
    rt_thread_mdelay(1000);
}
MSH_CMD_EXPORT(lcd_test_graphics, "Test LCD graphics drawing");
