#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "SSD1306";

// Configurações I2C
#define I2C_MASTER_SCL_IO           20    // GPIO para SCL
#define I2C_MASTER_SDA_IO           21    // GPIO para SDA
#define I2C_MASTER_NUM              0     // Número da porta I2C
#define I2C_MASTER_FREQ_HZ          400000 // Frequência I2C
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

// Configurações do SSD1306
#define SSD1306_I2C_ADDR            0x3C  // Endereço I2C (pode ser 0x3D)
#define SSD1306_WIDTH               128
#define SSD1306_HEIGHT              64
#define SSD1306_PAGES               8     // 64/8 = 8 páginas

// Comandos do SSD1306
#define SSD1306_CMD_DISPLAY_OFF     0xAE
#define SSD1306_CMD_DISPLAY_ON      0xAF
#define SSD1306_CMD_SET_CONTRAST    0x81
#define SSD1306_CMD_ENTIRE_DISPLAY_ON 0xA5
#define SSD1306_CMD_NORMAL_DISPLAY  0xA6
#define SSD1306_CMD_INVERT_DISPLAY  0xA7
#define SSD1306_CMD_SET_MULTIPLEX   0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET 0xD3
#define SSD1306_CMD_SET_START_LINE  0x40
#define SSD1306_CMD_SEGMENT_REMAP   0xA1
#define SSD1306_CMD_COM_SCAN_DEC    0xC8
#define SSD1306_CMD_SET_COM_PINS    0xDA
#define SSD1306_CMD_SET_CLOCK_DIV   0xD5
#define SSD1306_CMD_SET_PRECHARGE   0xD9
#define SSD1306_CMD_SET_VCOM_DETECT 0xDB
#define SSD1306_CMD_CHARGE_PUMP     0x8D
#define SSD1306_CMD_MEMORY_MODE     0x20
#define SSD1306_CMD_SET_COLUMN_ADDR 0x21
#define SSD1306_CMD_SET_PAGE_ADDR   0x22

// Buffer do display
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_PAGES];

// Font simples 8x8
static const uint8_t font8x8[95][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // espaço
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // !
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // #
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // $
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // %
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // &
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // (
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // )
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // *
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x06, 0x00}, // ,
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // .
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // /
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // 0
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // 1
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // 2
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // 3
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // 4
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // 5
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // 6
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // 7
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // 8
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // 9
    // Adicione mais caracteres conforme necessário...
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // :
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x06, 0x00}, // ;
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // <
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // =
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // >
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // ?
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // @
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // A
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // B
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // C
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // D
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // E
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // F
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // G
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // H
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // I
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // J
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // K
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // L
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // M
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // N
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // O
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // P
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // Q
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // R
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // S
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // T
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // X
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // Y
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // Z
};

// Função para enviar comando I2C
static esp_err_t ssd1306_write_command(uint8_t cmd) {
    i2c_cmd_handle_t cmd_link = i2c_cmd_link_create();
    i2c_master_start(cmd_link);
    i2c_master_write_byte(cmd_link, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_link, 0x00, true); // Co = 0, D/C = 0
    i2c_master_write_byte(cmd_link, cmd, true);
    i2c_master_stop(cmd_link);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_link, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_link);
    return ret;
}

// Função para enviar dados I2C
static esp_err_t ssd1306_write_data(uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd_link = i2c_cmd_link_create();
    i2c_master_start(cmd_link);
    i2c_master_write_byte(cmd_link, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_link, 0x40, true); // Co = 0, D/C = 1
    i2c_master_write(cmd_link, data, len, true);
    i2c_master_stop(cmd_link);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_link, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_link);
    return ret;
}

// Inicializar I2C
static void i2c_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
}

// Inicializar display SSD1306
static void ssd1306_init(void) {
    ESP_LOGI(TAG, "Inicializando SSD1306...");
    
    // Sequência de inicialização
    ssd1306_write_command(SSD1306_CMD_DISPLAY_OFF);
    ssd1306_write_command(SSD1306_CMD_SET_CLOCK_DIV);
    ssd1306_write_command(0x80);
    ssd1306_write_command(SSD1306_CMD_SET_MULTIPLEX);
    ssd1306_write_command(0x3F); // 128x64
    ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_OFFSET);
    ssd1306_write_command(0x00);
    ssd1306_write_command(SSD1306_CMD_SET_START_LINE | 0x00);
    ssd1306_write_command(SSD1306_CMD_CHARGE_PUMP);
    ssd1306_write_command(0x14);
    ssd1306_write_command(SSD1306_CMD_MEMORY_MODE);
    ssd1306_write_command(0x00);
    ssd1306_write_command(SSD1306_CMD_SEGMENT_REMAP | 0x01);
    ssd1306_write_command(SSD1306_CMD_COM_SCAN_DEC);
    ssd1306_write_command(SSD1306_CMD_SET_COM_PINS);
    ssd1306_write_command(0x12);
    ssd1306_write_command(SSD1306_CMD_SET_CONTRAST);
    ssd1306_write_command(0xCF);
    ssd1306_write_command(SSD1306_CMD_SET_PRECHARGE);
    ssd1306_write_command(0xF1);
    ssd1306_write_command(SSD1306_CMD_SET_VCOM_DETECT);
    ssd1306_write_command(0x40);
    ssd1306_write_command(SSD1306_CMD_ENTIRE_DISPLAY_ON);
    ssd1306_write_command(SSD1306_CMD_NORMAL_DISPLAY);
    ssd1306_write_command(SSD1306_CMD_DISPLAY_ON);
    
    ESP_LOGI(TAG, "SSD1306 inicializado com sucesso!");
}

// Limpar buffer
void ssd1306_clear_buffer(void) {
    memset(ssd1306_buffer, 0, sizeof(ssd1306_buffer));
}

// Atualizar display
void ssd1306_update_display(void) {
    ssd1306_write_command(SSD1306_CMD_SET_COLUMN_ADDR);
    ssd1306_write_command(0);
    ssd1306_write_command(127);
    ssd1306_write_command(SSD1306_CMD_SET_PAGE_ADDR);
    ssd1306_write_command(0);
    ssd1306_write_command(7);
    
    ssd1306_write_data(ssd1306_buffer, sizeof(ssd1306_buffer));
}

// Definir pixel
void ssd1306_set_pixel(int x, int y, bool on) {
    if (x >= 0 && x < SSD1306_WIDTH && y >= 0 && y < SSD1306_HEIGHT) {
        if (on) {
            ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
        } else {
            ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
        }
    }
}

// Desenhar caractere
void ssd1306_draw_char(int x, int y, char c) {
    if (c < 32 || c > 126) c = 32; // Espaço para caracteres inválidos
    int index = c - 32;
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            bool pixel = (font8x8[index][i] >> j) & 1;
            ssd1306_set_pixel(x + j, y + i, pixel);
        }
    }
}

// Desenhar string
void ssd1306_draw_string(int x, int y, const char* str) {
    while (*str) {
        ssd1306_draw_char(x, y, *str);
        x += 8;
        str++;
    }
}

// Desenhar linha
void ssd1306_draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        ssd1306_set_pixel(x0, y0, true);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Desenhar retângulo
void ssd1306_draw_rect(int x, int y, int w, int h, bool filled) {
    if (filled) {
        for (int i = x; i < x + w; i++) {
            for (int j = y; j < y + h; j++) {
                ssd1306_set_pixel(i, j, true);
            }
        }
    } else {
        ssd1306_draw_line(x, y, x + w - 1, y);
        ssd1306_draw_line(x + w - 1, y, x + w - 1, y + h - 1);
        ssd1306_draw_line(x + w - 1, y + h - 1, x, y + h - 1);
        ssd1306_draw_line(x, y + h - 1, x, y);
    }
}
