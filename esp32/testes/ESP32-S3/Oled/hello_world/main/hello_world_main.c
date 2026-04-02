#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "ssd1306.h"

#define TAG "OLED_MENU"

// Definições do I2C
#define I2C_MASTER_SCL_IO           17      // Pino SCL
#define I2C_MASTER_SDA_IO            18      // Pino SDA
#define I2C_MASTER_NUM               I2C_NUM_0       // Porta I2C
#define I2C_MASTER_FREQ_HZ           400000          // Frequência I2C
#define I2C_MASTER_TX_BUF_DISABLE    0               // Buffer de transmissão desabilitado
#define I2C_MASTER_RX_BUF_DISABLE    0               // Buffer de recepção desabilitado

// Definições dos botões
#define BUTTON_UP_GPIO               GPIO_NUM_2
#define BUTTON_DOWN_GPIO             GPIO_NUM_3
#define BUTTON_SELECT_GPIO           GPIO_NUM_4

// Estados do menu
typedef enum {
    MENU_OPTION_1,
    MENU_OPTION_2,
    MENU_OPTION_3,
    MENU_OPTION_4,
    MENU_TOTAL_OPTIONS
} menu_option_t;

static menu_option_t current_option = MENU_OPTION_1;
static SSD1306_t dev;

// Inicialização do I2C
static void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                      I2C_MASTER_TX_BUF_DISABLE, 
                      I2C_MASTER_RX_BUF_DISABLE, 0));
}

// Inicialização dos botões
static void init_buttons() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_UP_GPIO) | 
                        (1ULL << BUTTON_DOWN_GPIO) | 
                        (1ULL << BUTTON_SELECT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

// Verifica se um botão foi pressionado
static bool is_button_pressed(int gpio_num) {
    static uint32_t button_last_time[GPIO_NUM_MAX] = {0};
    
    if (gpio_get_level(gpio_num) == 0) { // Botão pressionado (LOW)
        uint32_t now = esp_timer_get_time() / 1000; // ms
        if ((now - button_last_time[gpio_num]) > 200) { // Debounce 200ms
            button_last_time[gpio_num] = now;
            return true;
        }
    }
    return false;
}

// Desenha o menu no display
static void draw_menu() {
    ssd1306_clear_screen(&dev, false);
    
    ssd1306_display_text(&dev, 0, "Menu Principal", 14, false);
    
    // Desenha as opções do menu
    for (int i = 0; i < MENU_TOTAL_OPTIONS; i++) {
        char line[20];
        const char* prefix = (i == current_option) ? "> " : "  ";
        
        switch(i) {
            case MENU_OPTION_1: snprintf(line, sizeof(line), "%sOpcao 1", prefix); break;
            case MENU_OPTION_2: snprintf(line, sizeof(line), "%sOpcao 2", prefix); break;
            case MENU_OPTION_3: snprintf(line, sizeof(line), "%sOpcao 3", prefix); break;
            case MENU_OPTION_4: snprintf(line, sizeof(line), "%sOpcao 4", prefix); break;
        }
        
        ssd1306_display_text(&dev, i+2, line, strlen(line), false);
    }
}

// Executa a ação da opção selecionada
static void execute_selected_option() {
    ssd1306_clear_screen(&dev, false);
    
    switch(current_option) {
        case MENU_OPTION_1:
            ssd1306_display_text(&dev, 2, "Executando", 10, false);
            ssd1306_display_text(&dev, 3, "Opcao 1...", 10, false);
            break;
        case MENU_OPTION_2:
            ssd1306_display_text(&dev, 2, "Executando", 10, false);
            ssd1306_display_text(&dev, 3, "Opcao 2...", 10, false);
            break;
        case MENU_OPTION_3:
            ssd1306_display_text(&dev, 2, "Executando", 10, false);
            ssd1306_display_text(&dev, 3, "Opcao 3...", 10, false);
            break;
        case MENU_OPTION_4:
            ssd1306_display_text(&dev, 2, "Executando", 10, false);
            ssd1306_display_text(&dev, 3, "Opcao 4...", 10, false);
            break;
        default:
            break;
    }
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    draw_menu();
}

void app_main(void) {
    // Inicializa I2C e display
    i2c_master_init();
    init_buttons();
    
    // Configura display OLED
    ssd1306_init(&dev, 128, 64, I2C_MASTER_NUM, false);
    ssd1306_clear_screen(&dev, false);
    
    // Desenha o menu inicial
    draw_menu();
    
    while (1) {
        // Verifica botões
        if (is_button_pressed(BUTTON_UP_GPIO)) {
            current_option = (current_option == 0) ? MENU_TOTAL_OPTIONS - 1 : current_option - 1;
            draw_menu();
        }
        else if (is_button_pressed(BUTTON_DOWN_GPIO)) {
            current_option = (current_option + 1) % MENU_TOTAL_OPTIONS;
            draw_menu();
        }
        else if (is_button_pressed(BUTTON_SELECT_GPIO)) {
            execute_selected_option();
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}