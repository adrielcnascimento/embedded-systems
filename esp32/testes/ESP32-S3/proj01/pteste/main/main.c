#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "ssd1306.h"

// Definição dos pinos
#define BUZZER_PIN     GPIO_NUM_6
#define BUTTON_1_PIN   GPIO_NUM_3
#define BUTTON_2_PIN   GPIO_NUM_4
#define OLED_SDA       GPIO_NUM_18
#define OLED_SCL       GPIO_NUM_17

// Configuração do PWM para o buzzer
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT // Resolução de 13 bits (0-8191)
#define LEDC_FREQUENCY  2000              // Frequência em Hz

// Configuração do menu
#define MENU_ITEMS     4
const char *menu_options[MENU_ITEMS] = {
    "1. Ligar LED",
    "2. Desligar LED",
    "3. Ajustar Brilho",
    "4. Configuracoes"
};

// Variáveis globais
uint8_t current_menu_item = 0;
uint32_t last_time_button1 = 0;
uint32_t last_time_button2 = 0;
SSD1306_t dev;
const uint32_t debounce_delay = 200; // 200ms
const uint32_t beep_duration = 250;  // 250ms

// Inicializa o PWM para o buzzer
void init_buzzer() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_PIN,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Toca um beep no buzzer
void beep(uint32_t duration_ms) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 4096); // 50% duty cycle
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Inicializa os botões
void init_buttons() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_1_PIN) | (1ULL << BUTTON_2_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// Inicializa o display OLED
void init_oled() {
    i2c_master_init(&dev, OLED_SDA, OLED_SCL, -1);
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);
}

// Atualiza o display com o menu atual
void update_display() {
    ssd1306_clear_screen(&dev, false);
    
    // Título
    ssd1306_display_text(&dev, 0, "Menu Principal", 14, true);
    
    // Itens do menu
    for (uint8_t i = 0; i < MENU_ITEMS; i++) {
        if (i == current_menu_item) {
            // Item selecionado (invertido)
            char buffer[20];
            snprintf(buffer, sizeof(buffer), ">%s", menu_options[i]);
            ssd1306_display_text(&dev, i+2, buffer, strlen(buffer), true);
        } else {
            // Itens normais
            ssd1306_display_text(&dev, i+2, menu_options[i], strlen(menu_options[i]), false);
        }
    }
    
    // Rodapé
    ssd1306_display_text(&dev, 7, "B1:Navega B2:Seleciona", 23, false);
}

// Verifica se um botão foi pressionado (com debounce)
bool button_pressed(gpio_num_t button_pin, uint32_t *last_press_time) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (gpio_get_level(button_pin) == 0) { // Botão pressionado (LOW)
        if (current_time - *last_press_time > debounce_delay) {
            *last_press_time = current_time;
            beep(beep_duration); // Toca o bip ao pressionar
            return true;
        }
    }
    return false;
}

// Processa a seleção do menu
void process_menu_selection(uint8_t selected_item) {
    char message[32];
    snprintf(message, sizeof(message), "Opcao %d selecionada!", selected_item + 1);
    
    ssd1306_clear_screen(&dev, false);
    ssd1306_display_text(&dev, 2, message, strlen(message), true);
    ssd1306_display_text(&dev, 4, "Pressione B1 para", 18, false);
    ssd1306_display_text(&dev, 5, "voltar ao menu", 14, false);
    
    // Aguarda o botão 1 ser pressionado para voltar
    while (!button_pressed(BUTTON_1_PIN, &last_time_button1)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    beep(beep_duration); // Toca o bip ao voltar
}

void app_main() {
    // Inicializa os periféricos
    init_buzzer();
    init_buttons();
    init_oled();
    
    // Mostra o menu inicial
    update_display();
    
    printf("Sistema iniciado. Use os botões para navegar no menu.\n");
    
    while (1) {
        // Navegação no menu com botão 1
        if (button_pressed(BUTTON_1_PIN, &last_time_button1)) {
            current_menu_item = (current_menu_item + 1) % MENU_ITEMS;
            update_display();
        }
        
        // Seleção com botão 2
        if (button_pressed(BUTTON_2_PIN, &last_time_button2)) {
            process_menu_selection(current_menu_item);
            update_display();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}