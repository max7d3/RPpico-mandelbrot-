#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"


#include "st7735/st7735.h"

#define WIDTH 160
#define HEIGHT 80
#define MAX_ITER 500


typedef struct RgbColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

} rgb_t;

typedef struct HsvColor
{
    uint8_t h;
    uint8_t s;
    uint8_t v;

} hsv_t;



rgb_t hsv_to_rgb(hsv_t hsv);
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) ;


// Smaller zoom value = bigger zoom
// Usable x/yPos -2 to 2
void draw_mandelbrot(double zoom, float xPos, float yPos);

int main() 
{
    // OVERCLOCK TO 250MHz 
    //set_sys_clock_khz(250000, false);

    stdio_init_all();

	LCD_Init();
	LCD_SetWindow(0, 0, 160, 80);
    LCD_Clear(ST7735_BLACK);
    LCD_Update();
 
    double zoom = 0.02;

    draw_mandelbrot(zoom, -1, -0);

    LCD_Update();


    while (1)
    {
        LCD_Clear(ST7735_BLACK);
        draw_mandelbrot(zoom, -0.7920001, -0.1424);
        zoom = zoom*0.9;
        LCD_Update();
    }

}


uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) 
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

rgb_t hsv_to_rgb(hsv_t hsv)
{
    rgb_t rgb;
    uint8_t region, p, q, t;
    uint16_t h, s, v, remainder;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }


    h = hsv.h;
    s = hsv.s;
    v = hsv.v;

    region = h / 43;
    remainder = (h - (region * 43)) * 6; 

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = v;
            rgb.b = t;
            break;
        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = v;
            break;
        default:
            rgb.r = v;
            rgb.g = p;
            rgb.b = q;
            break;
    }

    return rgb;
}

void draw_mandelbrot(double zoom, float xPos, float yPos)
{
    double zx, zy, cX, cY, tmp;


    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            zx = zy = 0;
            cX = xPos + (x - (WIDTH >> 1)) * zoom;
            cY = yPos + (y - (HEIGHT >> 1)) * zoom;

            int iter;

            for (iter = 0; iter < MAX_ITER && zx * zx + zy * zy < 4; iter++)
            {
                tmp = zx * zx - zy * zy + cX;
                zy = 2.0 * zx * zy + cY;
                zx = tmp;
            }

            if (iter == MAX_ITER)
            {
                LCD_DrawPixel(x, y, ST7735_BLACK);
            }
            else
            {
                hsv_t colors_hsv = {255, 255, 255};
                colors_hsv.h = map(iter, 0, MAX_ITER, 0, 255);

                rgb_t colors_rgb = hsv_to_rgb(colors_hsv);

                LCD_DrawPixel(x, y, color565(colors_rgb.r, colors_rgb.g, colors_rgb.b));
            }
        }
    }
} 



