
#include <pico/stdlib.h>
#include <pico/binary_info.h>
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
        printf("%-14s: 0x%04x\n", regs[i].name, (val[0] << 8) | val[1]);
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

    /* Set Bus ADC to 128 samples averaging */
    cfg |= (0b1111 << 7);

    /* Set Shunt ADC to 128 samples averaging */
    cfg |= (0b1111 << 3);

    /* Set mode to Shunt and Bus, continuous */
    cfg |= 0b111;

    uint8_t cfg_reg[3];
    cfg_reg[0] = INA219_REG_CFG;
    cfg_reg[1] = (cfg >> 8) & 0xff;
    cfg_reg[2] = cfg & 0xff;

    i2c_write_blocking(i2c0, INA219_ADDRESS, cfg_reg, 3, false);

    printf("INA219 registers before calibration:\n");
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

    printf("INA219 registers after calibration:\n");
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


int main( void )
{
    stdio_init_all();

    ina219_init();

    gpio_init(25);
    gpio_set_dir(25, true);

    for (;;)
    {
        sleep_ms(1000);
        gpio_xor_mask(1 << 25);

        printf("Amps: %.4f\n", ina219_measure_amps());
    }
    
    return 0;
}
