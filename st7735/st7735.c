#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "st7735.h"

uint8_t lcd_rotation = LCD_DEFAULT_ROTATION;

uint16_t lcd_buf[LCD_BUFFER_SIZE];

uint lcd_dma_channel;
dma_channel_config lcd_dma_cnofig;
bool lcd_dma_busy;

static void _LCD_SPI_Send(uint8_t byte)
{
    spi_write_blocking(LCD_SPI, &byte, 1);
}


static void _LCD_SendCommand(uint8_t cmd, const uint8_t *arg, uint8_t arg_len)
{
    LCD_CS_LOW;
    LCD_DC_LOW;

    _LCD_SPI_Send(cmd);

    LCD_DC_HIGH;

    if(arg_len > 0)
    {
        for (uint8_t i = 0; i < arg_len; i++)
        {
            _LCD_SPI_Send(*arg++); // Tu moze byc jakis problem
        }
    }
    
    LCD_CS_HIGH;
}

void _LCD_DMA_Handler()
{
    while(spi_is_busy(LCD_SPI));
    lcd_dma_busy = false;
    LCD_CS_HIGH;
    dma_hw->ints0 = 1u << lcd_dma_channel;
    //dma_channel_set_read_addr(lcd_dma_channel, lcd_buf, false);
}


void LCD_Init(void)
{

    uint temp = spi_init(LCD_SPI, LCD_SPI_SPEED);

    gpio_set_function(LCD_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SPI_CLK_PIN,GPIO_FUNC_SPI);

    gpio_init(LCD_SPI_CS_PIN);
    gpio_set_dir(LCD_SPI_CS_PIN, GPIO_OUT);
    gpio_put(LCD_SPI_CS_PIN, 1);                       

    gpio_init(LCD_SPI_DC_PIN);
    gpio_set_dir(LCD_SPI_DC_PIN, GPIO_OUT);
    gpio_put(LCD_SPI_DC_PIN, 0);                        

    gpio_init(LCD_SPI_RS_PIN);
    gpio_set_dir(LCD_SPI_RS_PIN, GPIO_OUT);
    gpio_put(LCD_SPI_RS_PIN, 0);


    LCD_RS_LOW;
    sleep_ms(200);
    LCD_RS_HIGH;
    sleep_ms(20);

    _LCD_SendCommand(ST7735_SLPOUT, 0, 0);
    sleep_ms(100);

    _LCD_SendCommand(ST7735_INVON, 0, 0);

    //const uint8_t arg_set0[] = {0x01, 0x0, 0x0};
    const uint8_t arg_set0[] = {0x01,0x2C,0x2D};
    _LCD_SendCommand(ST7735_FRMCTR1, arg_set0, 3);
    _LCD_SendCommand(ST7735_FRMCTR2, arg_set0, 3);

    //const uint8_t arg_set1[] = {0x01, 0x0, 0x0, 0x01, 0x0, 0x0};
    const uint8_t arg_set1[] = {0x01,0x2C,0x2D, 0x01,0x2C,0x2D};
    _LCD_SendCommand(ST7735_FRMCTR3, arg_set1, 6);

    const uint8_t arg_set2[] = {0x07};
    _LCD_SendCommand(0x4B, arg_set2, 1);

    const uint8_t arg_set3[] = {0x62, 0x02, 0x04};
    _LCD_SendCommand(ST7735_PWCTR1, arg_set3, 3);

    const uint8_t arg_set4[] = {0xC0};
    _LCD_SendCommand(ST7735_PWCTR2, arg_set4, 1);

    const uint8_t arg_set5[] = {0x0D, 0x00};
    _LCD_SendCommand(ST7735_PWCTR3, arg_set5, 2);

    const uint8_t arg_set6[] = {0x8D, 0x6A};
    _LCD_SendCommand(ST7735_PWCTR4, arg_set6, 2);

    const uint8_t arg_set7[] = {0x8D, 0xFE};
    _LCD_SendCommand(ST7735_PWCTR5, arg_set7, 2);

    const uint8_t arg_set8[] = {0x0E};
    _LCD_SendCommand(ST7735_VMCTR1, arg_set8, 1);

    const uint8_t arg_set9[] = {0x10, 0x0E, 0x02, 0x03, 0x0E, 0x07, 0x02, 0x07, 0x0A, 0x12, 0x27, 0x37, 0x00, 0x0D, 0x0E, 0x10};
    _LCD_SendCommand(ST7735_GMCTRP1, arg_set9, 16);

    const uint8_t arg_set10[] = {0x10, 0x0E, 0x03, 0x03, 0x0F, 0x06, 0x02, 0x08, 0x0A, 0x13, 0x26, 0x36, 0x00, 0x0D, 0x0E, 0x10};
    _LCD_SendCommand(ST7735_GMCTRN1, arg_set10, 16);

    const uint8_t arg_set11[] = {0x05};
    _LCD_SendCommand(ST7735_COLMOD, arg_set11, 1);

    LCD_SetRotation(LCD_DEFAULT_ROTATION);

    _LCD_SendCommand(ST7735_DISPON, 0, 0);

    spi_set_format(LCD_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);


    lcd_dma_channel = dma_claim_unused_channel(true);
    lcd_dma_cnofig = dma_channel_get_default_config(lcd_dma_channel);

    channel_config_set_transfer_data_size(&lcd_dma_cnofig, DMA_SIZE_16);
    channel_config_set_dreq(&lcd_dma_cnofig, spi_get_dreq(LCD_SPI, true));
    dma_channel_configure(lcd_dma_channel, &lcd_dma_cnofig,
                          &spi_get_hw(LCD_SPI)->dr, // write address
                          lcd_buf, // read address
                          LCD_BUFFER_SIZE, // element count
                          false); //don't start yet

    dma_channel_set_irq0_enabled(lcd_dma_channel, true);

    irq_set_exclusive_handler(DMA_IRQ_0, _LCD_DMA_Handler);
    irq_set_enabled(DMA_IRQ_0, true);

}

void LCD_SetRotation(uint8_t rotation)
{
    uint8_t arg[] = {0x00};

    switch (rotation)
    {
    case 0:
        arg[0] = (0x00 | (1 << 3));
        break;

    case 1:
        arg[0] = (0xC0 | (1 << 3));
        break;

    case 2:
        arg[0] = (0x70 | (1 << 3));
        break;

    case 3:
        arg[0] = (0xA0 | (1 << 3));
        break;

    default:
        return;
    }

    _LCD_SendCommand(ST7735_MADCTL, arg, 1);
    lcd_rotation = rotation;
}

void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
    uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
    uint8_t arg0[4] = {0x00, 0x00, 0x00, 0x00};
    uint8_t arg1[4]= {0x00, 0x00, 0x00, 0x00};

    switch (lcd_rotation)
    {
    case 2:
        arg0[1] = x1 + 1;
        arg0[3] = x2 + 1;
        arg1[1] = y1 + 26;
        arg1[3] = y2 + 26;
        break;

    case 1:
        arg0[1] = x1 + 26;
        arg0[3] = x2 + 26;
        arg1[1] = y1 + 1;
        arg1[3] = y2 + 1;
        break;

    case 0:
        arg0[1] = x1 + 26;
        arg0[3] = x2 + 26;
        arg1[1] = y1 + 1;
        arg1[3] = y2 + 1;
        break;

    case 3:
        arg0[1] = x1 + 1;
        arg0[3] = x2 + 1;
        arg1[1] = y1 + 26;
        arg1[3] = y2 + 26;
        break;
    
    default:
        return;
    }
    
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    _LCD_SendCommand(ST7735_CASET, arg0, 4);
    _LCD_SendCommand(ST7735_RASET, arg1, 4);
    _LCD_SendCommand(ST7735_RAMWR, 0, 0);

    spi_set_format(LCD_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

}

void LCD_Wait(void)
{
    if (lcd_dma_busy == true)
    {
        dma_channel_wait_for_finish_blocking(lcd_dma_channel);
    }
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{

    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    lcd_buf[x + y * LCD_WIDTH] = color;

}

// void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
// {
    
//     LCD_SetWindow(x,y,1, 1);
//     LCD_CS_LOW;
//     spi_write16_blocking(LCD_SPI, &color, 1);
//     LCD_CS_HIGH;

// }

void LCD_Clear(uint16_t color)
{
    LCD_Wait();
    memset(lcd_buf, color, LCD_BUFFER_SIZE*2);


    // LCD_SetWindow(0, 0, 160, 80);

    // uint16_t temp[64];
    // memset(temp, color, 128);

    // LCD_CS_LOW;
    // for (size_t i = 0; i < LCD_BUFFER_SIZE / 64; i++)
    // {
    //     spi_write16_blocking(LCD_SPI, temp, 64);
    // }
    // LCD_CS_HIGH;
    
}

void LCD_Update(void)
{
    LCD_Wait();
    
    LCD_CS_LOW;

    dma_channel_transfer_from_buffer_now(lcd_dma_channel, (uint16_t*)lcd_buf, LCD_BUFFER_SIZE);
    lcd_dma_busy = true;

}