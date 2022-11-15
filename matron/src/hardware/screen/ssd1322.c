#include "ssd1322.h"

static int spidev_fd = 0;
static struct gpiod_chip * gpio_0;
static struct gpiod_line * gpio_dc;
static struct gpiod_line * gpio_reset;

int write_gpio(struct gpiod_line * line, uint8_t value){

    gpiod_line_set_value(line, value);
    return 0;
}

int open_spi() {
    uint8_t mode = SPI_MODE_0 | SPI_NO_CS;
    uint8_t bits_per_word = SPI0_BUS_WIDTH;
    uint8_t little_endian = 0;
    uint32_t speed_hz = 1200000000 / 64; // 18.75Mhz, 1200Mhz is the CPU speed.

    int fd = open(SPIDEV_0_0_PATH, O_RDWR | O_SYNC);

    if( fd < 0 ){
        fprintf(stderr, "(screen) couldn't open %s\n", SPIDEV_0_0_PATH);
        return -1;
    }

    int outcome = 0
    || ( ioctl(fd, SPI_IOC_WR_MODE, &mode)                   < 0 )
    || ( ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0 )
    || ( ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz)       < 0 )
    || ( ioctl(fd, SPI_IOC_WR_LSB_FIRST, &little_endian)     < 0 );
    if( outcome != 0 ){
       fprintf(stderr, "could not set SPI WR settings via IOC\n");
       close(fd);
       return -1;
    }

    return fd;
}

int ssd1322_write_command(uint8_t command, uint8_t data_len, ...) {
    va_list args;
    uint8_t cmd_buf[1];
    uint8_t data_buf[256];
    struct spi_ioc_transfer cmd_transfer = {0};
    struct spi_ioc_transfer data_transfer = {0};

    if( spidev_fd <= 0 ){
        fprintf(stderr, "ssd1322_write_command: spidev not yet opened\n");
        return -1;
    }

    gpiod_line_set_value(gpio_dc, 0);

    cmd_buf[0] = command;
    cmd_transfer.tx_buf = (unsigned long) cmd_buf;
    cmd_transfer.len = (uint32_t) sizeof(cmd_buf);

    if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &cmd_transfer) < 0 ){
        fprintf(stderr, "could not send SPI command-message via ioctl().\n");
        return -1;
    }

    if( data_len > 0 ){
        gpiod_line_set_value(gpio_dc, 1);

        va_start(args, data_len);

        for( uint8_t i = 0; i < data_len; i++ ){
            data_buf[i] = va_arg(args, int);
        }

        va_end(args);

        data_transfer.tx_buf = (unsigned long) data_buf;
        data_transfer.len = (uint32_t) data_len;

        if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &data_transfer) < 0 ){
            fprintf(stderr, "could not send SPI data-message via ioctl().\n");
            return -1;
        }
    }

    return 0;
}

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
#define write_command_with_data(x, ...) \
    (ssd1322_write_command(x, NUMARGS(__VA_ARGS__), __VA_ARGS__))
#define write_command(x) \
    (ssd1322_write_command(x, 0, 0))

void ssd1322_init() {

    spidev_fd = open_spi(SPIDEV_0_0_PATH);
    if( spidev_fd < 0 ){
        fprintf(stderr, "ssd1322_init: couldn't open %s.\n", SPIDEV_0_0_PATH);
        return;
    }

    gpio_0 = gpiod_chip_open_by_name(SSD1322_DC_AND_RESET_GPIO_CHIP);
    gpio_dc = gpiod_chip_get_line(gpio_0, SSD1322_DC_GPIO_LINE);
    gpio_reset = gpiod_chip_get_line(gpio_0, SSD1322_RESET_GPIO_LINE);

    gpiod_line_request_output(gpio_dc, "D/C", 0);
    gpiod_line_request_output(gpio_reset, "RST", 0);

    // SSD1322 Reference Document (v1.2) P 16/60
    // "Keep this pin pull HIGH during normal operation"
    gpiod_line_set_value(gpio_reset, 1);

    write_command(SSD1322_SET_DISPLAY_OFF);
    write_command(SSD1322_SET_DEFAULT_LINEAR_GRAY_SCALE);
    write_command_with_data(SSD1322_SET_OSCILLATOR_FREQUENCY, 0xF2);
    write_command_with_data(SSD1322_SET_MULTIPLEX_RATIO, 0x3F);
    write_command_with_data(SSD1322_SET_DISPLAY_OFFSET, 0x00);
    write_command_with_data(SSD1322_SET_DISPLAY_START_LINE, 0x00);
    write_command_with_data(SSD1322_SET_VDD_REGULATOR, 0x01);
    write_command_with_data(SSD1322_SET_DUAL_COMM_LINE_MODE, 0x16, 0x11);
    write_command_with_data(SSD1322_SET_DISPLAY_ENHANCEMENT_A, 0xA0, 0xFD);
    write_command_with_data(SSD1322_SET_CONTRAST_CURRENT, 0x7F);
    write_command_with_data(SSD1322_MASTER_CURRENT_CONTROL, 0x0F);
    write_command_with_data(SSD1322_SET_PHASE_LENGTH, 0xF2);
    write_command_with_data(SSD1322_SET_PRECHARGE_VOLTAGE, 0x1F);
    write_command_with_data(SSD1322_SET_VCOMH_VOLTAGE, 0x04);
    write_command_with_data(SSD1322_SET_COLUMN_ADDRESS, 28, 91);
    write_command_with_data(SSD1322_SET_ROW_ADDRESS, 0, 63);
    write_command(SSD1322_WRITE_RAM_COMMAND); // set GDDRAM for write, doesn't
                                              // effect other commands.
    write_command(SSD1322_SET_DISPLAY_MODE_NORMAL);
    write_command(SSD1322_SET_DISPLAY_ON);
}

void ssd1322_deinit(){
    if( spidev_fd > 0 ){
        gpiod_line_release(gpio_reset);
        gpiod_line_release(gpio_dc);
        gpiod_chip_close(gpio_0);
        close(spidev_fd);
    }
}

void ssd1322_update(uint8_t * buf, uint16_t buf_len){
    struct spi_ioc_transfer transfer = {0};

    if( spidev_fd <= 0 ){
        fprintf(stderr, "%s: spidev not yet opened.\n", __func__);
        return;
    }

    if( buf_len > 8192 ){
        fprintf(stderr, "%s: buf_len greater than screen GDDRAM", __func__);
    }


    gpiod_line_set_value(gpio_dc, 1);

    // The spidev module has a buffer size limit of 4096.
    // Setting it in /boot/cmdline.txt like the internet suggests didn't
    // work for me. Instead, just send two separate SPI transactions.

    transfer.tx_buf = (unsigned long) buf;
    transfer.len = (uint32_t) buf_len / 2;

    if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &transfer) < 0 ){
        fprintf(stderr, "%s: SPI data transfer 1 failed.\n", __func__);
        return;
    }
   
    transfer.tx_buf = (unsigned long) (buf + (buf_len / 2));
    
    if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &transfer) < 0 ){
        fprintf(stderr, "%s: SPI data transfer 2 failed.\n", __func__);
        return;
    }
}

#define LIMIT(x,y) ( (x > y) ? y : x )

ssd1322_set_gamma(struct ssd1322_grayscale_table_t *t){
    write_command_with_data(
            SSD1322_SET_GRAY_SCALE_TABLE,
            LIMIT(t->GS0, 0),
            LIMIT(t->GS1,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS2,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS3,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS4,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS5,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS6,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS7,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS8,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS9,  SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS10, SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS11, SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS12, SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS13, SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS14, SSD1322_GRAYSCALE_MAX_VALUE),
            LIMIT(t->GS15, SSD1322_GRAYSCALE_MAX_VALUE)
    );
}

ssd1322_set_brightness(uint8_t b){
    write_command_with_data(SSD1322_SET_PRECHARGE_VOLTAGE, b);
}