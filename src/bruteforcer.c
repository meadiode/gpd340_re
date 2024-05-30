
#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <stdio.h>

#define I2C_SCL_PIN 17
#define I2C_SDA_PIN 16

#define INA219_ADDRESS   0x40

#define INA219_REG_CFG   0x00
#define INA219_REG_SV    0x01
#define INA219_REG_BV    0x02
#define INA219_REG_PWR   0x03
#define INA219_REG_AMP   0x04
#define INA219_REG_CAL   0x05
#define INA219_MAGIC_VAL 0.04096f

#define R_SHUNT           0.1f
#define EXPECTED_MAX_AMPS 0.5f
#define CURRENT_LSB       (EXPECTED_MAX_AMPS / 32768.0f)


#define GPD340_PIN1 10
#define GPD340_PIN2 11
#define GPD340_PIN3 12
#define GPD340_PIN4 13
#define GPD340_PIN7 14
#define GPD340_PIN8 15

#define GPD340_PINS_MASK ((1 << GPD340_PIN1) | (1 << GPD340_PIN2) | \
                          (1 << GPD340_PIN3) | (1 << GPD340_PIN4) | \
                          (1 << GPD340_PIN7) | (1 << GPD340_PIN8))

static void ina219_printout_registers(void)
{
    static const struct
    {
        uint8_t reg;
        const char* name;
    
    } regs[6] = 
    {
        {INA219_REG_CFG, "Configuration"},
        {INA219_REG_SV, "Shunt voltage"},
        {INA219_REG_BV, "Bus voltage"},
        {INA219_REG_PWR, "Power"},
        {INA219_REG_AMP, "Current"},
        {INA219_REG_CAL, "Calibration"},
    };

    uint8_t val[2] = {0};
    
    for (uint8_t i = 0; i < 6; i++)
    {
        i2c_write_blocking(i2c0, INA219_ADDRESS, &regs[i].reg, 1, true);
        i2c_read_blocking(i2c0, INA219_ADDRESS, val, 2, false);
        printf("# %-14s: 0x%04x\n", regs[i].name, (val[0] << 8) | val[1]);
    }

    printf("\n\n");
}


static void ina219_init(void)
{
    i2c_init(i2c0, 400 * 1000);

    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);
    gpio_pull_up(I2C_SDA_PIN);
    bi_decl(bi_2pins_with_func(I2C_SCL_PIN, I2C_SDA_PIN, GPIO_FUNC_I2C));

    sleep_ms(100);

    uint16_t cfg = 0x0;

    /* Set bus voltage range to 16V */
    cfg |= (0b1 << 13);

    /* Set PGA range to +-80mV */
    cfg |= (0b01 << 11);

    /* Set Bus ADC to 128-sample averaging */
    cfg |= (0b1111 << 7);

    /* Set Shunt ADC to 128-sample averaging */
    cfg |= (0b1111 << 3);

    /* Set mode to Shunt and Bus, continuous */
    cfg |= 0b111;

    uint8_t cfg_reg[3];
    cfg_reg[0] = INA219_REG_CFG;
    cfg_reg[1] = (cfg >> 8) & 0xff;
    cfg_reg[2] = cfg & 0xff;

    i2c_write_blocking(i2c0, INA219_ADDRESS, cfg_reg, 3, false);

    printf("# INA219 registers before calibration:\n");
    ina219_printout_registers();

    uint8_t reg = INA219_REG_CFG;
    uint8_t val[2] = {0};

    uint16_t cal_v = INA219_MAGIC_VAL / (CURRENT_LSB * R_SHUNT);

    uint8_t cal_reg[3] = {0};
    cal_reg[0] = INA219_REG_CAL;
    cal_reg[1] = (cal_v >> 8) & 0xff;
    cal_reg[2] = cal_v & 0xff;
    i2c_write_blocking(i2c0, INA219_ADDRESS, cal_reg, 3, false);

    sleep_ms(1000);

    printf("# INA219 registers after calibration:\n");
    ina219_printout_registers();
}


static float ina219_measure_amps(void)
{
    const uint8_t reg = INA219_REG_AMP;
    uint8_t val[2] = {0};

    i2c_write_blocking(i2c0, INA219_ADDRESS, &reg, 1, true);
    i2c_read_blocking(i2c0, INA219_ADDRESS, val, 2, false);
    
    float res = (float)((val[0] << 8) | val[1]) * CURRENT_LSB;

    return res;
}


static void run_combination(uint32_t clk_pin, uint32_t data_pin,
                            uint32_t fixed_pins[4], uint8_t fixed_pins_state)
{
    uint16_t i, j, n;

    gpio_put_masked(GPD340_PINS_MASK, 0x0);

    multicore_fifo_pop_blocking();

    /* Set up "fixed" pins */ 
    for (i = 0; i < 4; i++)
    {
        gpio_put(fixed_pins[i], ((fixed_pins_state >> i) & 1));
    }

    static const uint8_t data_patterns[10] = {
        0b00000001,
        0b00000011,
        0b00000111,
        0b00001111,
        0b00011111,
        0b00111111,
        0b01111111,
        0b11111111,
        0b01010101,
        0b10101010,
    };

    for (i = 0; i < 10; i++)
    {
        uint8_t data = data_patterns[i];

        for (n = 0; n < 1000; n++)
        {
            for (j = 0; j < 8; j++)
            {
                gpio_put(data_pin, (data >> j) & 1);
             
                gpio_put(clk_pin, true);
                sleep_us(1);
                gpio_put(clk_pin, false);
                sleep_us(0);
            }
            sleep_ms(1);
        }
    }

    multicore_fifo_push_blocking(1);
}


void core1_entry(void)
{
    static const uint32_t pins[6] = {GPD340_PIN1, GPD340_PIN2, GPD340_PIN3,
                                     GPD340_PIN4, GPD340_PIN7, GPD340_PIN8};

    gpio_init_mask(GPD340_PINS_MASK);
    gpio_set_dir_masked(GPD340_PINS_MASK, 0xffffffff);
    gpio_put_masked(GPD340_PINS_MASK, 0x0);


    uint8_t clk_pin_idx, data_pin_idx, const_pin_idx, csp_id;
    uint8_t csp_state;
    uint32_t fixed_pins[4] = {0};

    for (clk_pin_idx = 0; clk_pin_idx < 6; clk_pin_idx++)
    {
        for (data_pin_idx = 0; data_pin_idx < 6; data_pin_idx++)
        {
            if (data_pin_idx == clk_pin_idx)
            {
                continue;
            }

            csp_id = 0;
            for (const_pin_idx = 0; const_pin_idx < 6; const_pin_idx++)
            {
                if (const_pin_idx == clk_pin_idx ||
                    const_pin_idx == data_pin_idx)
                {
                    continue;
                }

                fixed_pins[csp_id] = pins[const_pin_idx];
                csp_id++;
            }

            for (csp_state = 0; csp_state < 16; csp_state++)
            {
                run_combination(pins[clk_pin_idx], pins[data_pin_idx],
                                fixed_pins, csp_state);
            }
        }
    }

    multicore_fifo_push_blocking(0);

    for (;;)
    {
        sleep_ms(1000);
    }
}



int main(void)
{
    uint32_t combo_id = 0;
    uint32_t flag = 0;

    stdio_init_all();

    ina219_init();

    multicore_launch_core1(core1_entry);

    for (;;)
    {
        sleep_ms(1000);
    }

    for (;;)
    {
        multicore_fifo_push_blocking(1);
        printf("# Starting combination #%03d\n", combo_id);

        for (;;)
        {
            sleep_ms(200);
            printf("Cid: %03d, Amps: %.4f\n", combo_id, ina219_measure_amps());
        
            if (multicore_fifo_pop_timeout_us(1000, &flag))
            {
                break;
            }
        }

        if (flag == 0)
        {
            break;
        }
        combo_id++;
    }

    printf("# Bruteforcing complete!\n");
    gpio_put_masked(GPD340_PINS_MASK, 0x0);

    for (;;)
    {
        sleep_ms(1000);
    }
    
    return 0;
}
