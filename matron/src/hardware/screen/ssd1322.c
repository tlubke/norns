#include "ssd1322.h"

int ssd1322_write_command(uint8_t command, uint64_t data_len, ...) {

    bcm2835_gpio_write(SSD1322_GPIO_DC, LOW);

    char cmd_buf[] = { command };
    bcm2835_spi_writenb(cmd_buf, sizeof(cmd_buf));

    if( data_len > 0 ){
        bcm2835_gpio_write(SSD1322_GPIO_DC, HIGH);

        va_list args;
        va_start(args, data_len);

        char data[data_len];

        for( uint64_t i = 0; i < data_len; i++ ){
            data[i] = va_arg(args, int);
        }

        va_end(args);

        bcm2835_spi_writenb(data, data_len);
    }

    return 0;
}

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
#define write_command_with_data(x, ...) \
    (ssd1322_write_command(x, NUMARGS(__VA_ARGS__), __VA_ARGS__))
#define write_command(x) \
    (ssd1322_write_command(x, 0, 0))

void ssd1322_init() {

    bcm2835_init();

    bcm2835_gpio_fsel(SSD1322_GPIO_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(SSD1322_GPIO_RESET, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_write(SSD1322_GPIO_RESET, HIGH);

    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, HIGH); // Active high, pull low to select.
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);

    int rotate = 1; // TODO: get the real value.

    write_command(SSD1322_SET_DISPLAY_OFF);
    write_command(SSD1322_SET_DEFAULT_LINEAR_GRAY_SCALE);
    write_command_with_data(SSD1322_SET_OSCILLATOR_FREQUENCY, 0xF2);
    write_command_with_data(SSD1322_SET_MULTIPLEX_RATIO, 0x3F);
    write_command_with_data(SSD1322_SET_DISPLAY_OFFSET, 0x00);
    write_command_with_data(SSD1322_SET_DISPLAY_START_LINE, 0x00);
    write_command_with_data(SSD1322_SET_VDD_REGULATOR, 0x01);
    write_command_with_data(SSD1322_SET_DUAL_COMM_LINE_MODE, ( (rotate == 180) ? 0x04 : 0x16 ), 0x11);
    write_command_with_data(SSD1322_SET_DISPLAY_ENHANCEMENT_A, 0xA0, 0xFD);
    write_command_with_data(SSD1322_SET_CONTRAST_CURRENT, 0x7F);
    write_command_with_data(SSD1322_MASTER_CURRENT_CONTROL, 0x0F);
    write_command_with_data(SSD1322_SET_PHASE_LENGTH, 0xF2);
    write_command_with_data(SSD1322_SET_PRECHARGE_VOLTAGE, 0x1F);
    write_command_with_data(SSD1322_SET_VCOMH_VOLTAGE, 0x04);
    write_command(SSD1322_SET_DISPLAY_MODE_NORMAL);
    write_command(SSD1322_SET_DISPLAY_ON);
}

void ssd1322_end(){
    bcm2835_spi_end();
}

void ssd1322_update(char * buf, uint32_t buflen){
    write_command_with_data(SSD1322_SET_COLUMN_ADDRESS, 28, 91);
    write_command_with_data(SSD1322_SET_ROW_ADDRESS, 0, 63);
    write_command(SSD1322_WRITE_RAM_COMMAND);

    bcm2835_gpio_write(SSD1322_GPIO_DC, HIGH);

    bcm2835_spi_writenb(buf, buflen);
}