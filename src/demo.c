
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <stdio.h>
#include <string.h>

#include "font5x7_transposed.h"

#define GPD_DATA_PIN      13
#define GPD_BRCTRL_PIN    12
#define GPD_CLK_PINS_MASK1 (((1 << 9) - 1) << 14)
#define GPD_CLK_PINS_MASK2 (((1 << 3) - 1) << 26)

#define NCHARS 12

static const uint32_t clk_pin_char_map[NCHARS] = 
{
    15, 14, 16, 17, 18, 19, 20, 21, 22, 26, 27, 28
};


static void put_bitmap(uint8_t char_id, const uint8_t *data)
{
    uint32_t i, j;
    uint32_t clk_pin = clk_pin_char_map[char_id];

    /* Initial bit to start transmition */
    gpio_put(GPD_DATA_PIN, 1);
    gpio_put(clk_pin, 0);
    sleep_us(1);
    gpio_put(clk_pin, 1);
    sleep_us(1);

    /* Transmit 7x5 bitmap - 7 rows, 5 columns, bottom rows first */
    for (i = 0; i < 7; i++)
    {
        for (j = 0; j < 5; j++)
        {
            gpio_put(GPD_DATA_PIN, (data[6 - i] >> (4 - j) & 1)); 
            gpio_put(clk_pin, false);
            sleep_us(1);
            gpio_put(clk_pin, true);
            sleep_us(1);
        }
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
}


static void put_string(const uint8_t *str)
{
    uint32_t len = strnlen(str, NCHARS);
    
    for (uint32_t i = 0; i < len; i++)
    {
        put_bitmap(i, &font5x7[str[i] * FONT_BYTES_IN_CHAR]);
    }
}


static void clear(void)
{
    const uint8_t zeros[7] = {0};

    for (uint8_t i = 0; i < NCHARS; i++)
    {
        put_bitmap(i, zeros);
    }
}


int main(void)
{
    stdio_init_all();

    gpio_init(GPD_DATA_PIN);
    gpio_init_mask(GPD_CLK_PINS_MASK1);
    gpio_init_mask(GPD_CLK_PINS_MASK2);

    gpio_set_dir(GPD_DATA_PIN, true);
    gpio_set_dir_masked(GPD_CLK_PINS_MASK1, GPD_CLK_PINS_MASK1);
    gpio_set_dir_masked(GPD_CLK_PINS_MASK2, GPD_CLK_PINS_MASK2);

    gpio_init(GPD_BRCTRL_PIN);
    gpio_set_dir(GPD_BRCTRL_PIN, true);
    gpio_set_function(GPD_BRCTRL_PIN, GPIO_FUNC_PWM);
    gpio_pull_up(GPD_BRCTRL_PIN);

    uint slice_num = pwm_gpio_to_slice_num(GPD_BRCTRL_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 220);
    pwm_set_clkdiv(slice_num, 20.0);
    pwm_set_enabled(slice_num, true);

    clear();
    clear();

    uint8_t n = 0;


    uint8_t scrolltxt[] = "  HELLORLD!    HELLORLD!  ";
    uint32_t i, len = strlen(scrolltxt);

    for (;;)
    {
        put_string(scrolltxt);

        uint8_t c = scrolltxt[0];
        for (i = 0; i < len - 1; i++)
        {
            scrolltxt[i] = scrolltxt[i + 1];
        }
        scrolltxt[i] = c;
    
        sleep_ms(200);
    }
    
    return 0;
}
