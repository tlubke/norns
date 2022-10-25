#include "ssd1322.h"

int write_gpio(uint8_t chip, uint8_t line, uint8_t value){
    struct gpiohandle_data data = {
            .values = { value }
    };
    struct gpiohandle_request request = {
            .consumer_label = "N/A", // not used, could use an enum and do something to label GPIO.
            .fd = 0, // Set by GPIO_GET_LINEHANDLE_IOCTL.
            .flags = GPIOHANDLE_REQUEST_OUTPUT,
            .lines = 1,
            .lineoffsets = { line },
            .default_values = { 0 }, // If request doesn't receive data, pull LOW.
    };

    char device_name[32];
    sprintf(device_name, "/dev/gpiochip%d", chip);

    int fd = open(device_name, O_WRONLY | O_SYNC);

    if( fd < 0 ){
        fprintf(stderr, "(gpio) couldn't open %s\n", device_name);
        return -1;
    }

    if( ioctl( fd, GPIO_GET_LINEHANDLE_IOCTL, &request ) < 0){
        fprintf(stderr, "(gpio) couldn't complete gpio_get_linehandle for gpio%x %x=%x\n", chip, line, value);
        return -1;
    }

    close(fd);

    if( ioctl( request.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data ) < 0){
        fprintf(stderr, "(gpio) couldn't complete gpio_set_line_values for gpio%x %x=%x\n", chip, line, value);
        return -1;
    }

    close(request.fd);

    return 0;
}

int open_spi() {
    uint8_t mode = SPI_MODE_0 | SPI_NO_CS;
    uint8_t bits_per_word = SPI0_BUS_WIDTH;
    uint8_t little_endian = 0;
    uint32_t speed_hz = 1200000000 / 64; // 18.75Mhz, 1200Mhz is the CPU speed.

    int spidev_fd = open(SPI0_0_DEVICE_PATH, O_RDWR | O_SYNC);

    if( spidev_fd < 0 ){
        fprintf(stderr, "(screen) couldn't open %s\n", SPI0_0_DEVICE_PATH);
        return -1;
    }

    int outcome = 0
    || ( ioctl(spidev_fd, SPI_IOC_WR_MODE, &mode)                   < 0 )
    || ( ioctl(spidev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0 )
    || ( ioctl(spidev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz)       < 0 )
    || ( ioctl(spidev_fd, SPI_IOC_WR_LSB_FIRST, &little_endian)     < 0 );
    if( outcome != 0 ){
       fprintf(stderr, "could not set SPI WR settings via IOC\n");
       close(spidev_fd);
       return -1;
    }

    return spidev_fd;
}

int ssd1322_write_command(int fd, uint8_t command, uint64_t data_len, ...) {

    uint8_t cmd_buf[] = { command };

    write_gpio(SSD1322_DC_GPIO_CHIP, SSD1322_DC_GPIO_LINE, 0);

    struct spi_ioc_transfer command_transfer = {0};
    command_transfer.tx_buf = (unsigned long) cmd_buf;
    command_transfer.len = sizeof(cmd_buf);

    if( ioctl(fd, SPI_IOC_MESSAGE(1), &command_transfer) < 0 ){
        fprintf(stderr, "could not send SPI command-message via ioctl().\n");
        return -1;
    }

    if( data_len > 0 ){
        write_gpio(SSD1322_DC_GPIO_CHIP, SSD1322_DC_GPIO_LINE, 1);

        va_list args;
        va_start(args, data_len);

        uint8_t data[data_len];

        for( uint64_t i = 0; i < data_len; i++ ){
            data[i] = va_arg(args, int);
        }

        va_end(args);

        struct spi_ioc_transfer data_transfer = {0};
        data_transfer.tx_buf = (unsigned long) data;
        data_transfer.len = data_len;

        if( ioctl(fd, SPI_IOC_MESSAGE(1), &data_transfer) < 0 ){
            fprintf(stderr, "could not send SPI data-message via ioctl().\n");
            return -1;
        }
    }

    return 0;
}

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
#define write_command_with_data(x, y, ...) \
    (ssd1322_write_command(x, y, NUMARGS(__VA_ARGS__), __VA_ARGS__))
#define write_command(x, y) \
    (ssd1322_write_command(x, y, 0, 0))

void ssd1322_init() {

    // SSD1322 Reference Document (v1.2) P 16/60
    // "Keep this pin pull HIGH during normal operation"
    write_gpio(SSD1322_RESET_GPIO_CHIP, SSD1322_RESET_GPIO_LINE, 1);

    int fd = open_spi(SPI0_0_DEVICE_PATH);

    write_command(fd, SSD1322_SET_DISPLAY_OFF);
    write_command(fd, SSD1322_SET_DEFAULT_LINEAR_GRAY_SCALE);
    write_command_with_data(fd, SSD1322_SET_OSCILLATOR_FREQUENCY, 0xF2);
    write_command_with_data(fd, SSD1322_SET_MULTIPLEX_RATIO, 0x3F);
    write_command_with_data(fd, SSD1322_SET_DISPLAY_OFFSET, 0x00);
    write_command_with_data(fd, SSD1322_SET_DISPLAY_START_LINE, 0x00);
    write_command_with_data(fd, SSD1322_SET_VDD_REGULATOR, 0x01);
    write_command_with_data(fd, SSD1322_SET_DUAL_COMM_LINE_MODE, 0x16, 0x11);
    write_command_with_data(fd, SSD1322_SET_DISPLAY_ENHANCEMENT_A, 0xA0, 0xFD);
    write_command_with_data(fd, SSD1322_SET_CONTRAST_CURRENT, 0x7F);
    write_command_with_data(fd, SSD1322_MASTER_CURRENT_CONTROL, 0x0F);
    write_command_with_data(fd, SSD1322_SET_PHASE_LENGTH, 0xF2);
    write_command_with_data(fd, SSD1322_SET_PRECHARGE_VOLTAGE, 0x1F);
    write_command_with_data(fd, SSD1322_SET_VCOMH_VOLTAGE, 0x04);
    write_command(fd, SSD1322_SET_DISPLAY_MODE_ALL_ON);
    write_command(fd, SSD1322_SET_DISPLAY_ON);

    close(fd);
}
