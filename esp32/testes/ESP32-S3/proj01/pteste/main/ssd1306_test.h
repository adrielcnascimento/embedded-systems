// Função para testar pixels individuais
void test_pixels(void) {
    printf("Teste 1: Pixels individuais");
    ssd1306_clear_buffer();
    
    // Desenhar padrão de pixels
    for (int i = 0; i < 128; i += 4) {
        for (int j = 0; j < 64; j += 4) {
            ssd1306_set_pixel(i, j, true);
        }
    }
    
    ssd1306_update_display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

// Função para testar texto
void test_text(void) {
    printf("Teste 2: Texto");
    ssd1306_clear_buffer();
    
    ssd1306_draw_string(0, 0, "ESP32-SSD1306");
    ssd1306_draw_string(0, 16, "Teste de texto");
    ssd1306_draw_string(0, 32, "123456789");
    ssd1306_draw_string(0, 48, "!@#$%^&*()");
    
    ssd1306_update_display();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

// Função para testar linhas
void test_lines(void) {
    printf("Teste 3: Linhas");
    ssd1306_clear_buffer();
    
    // Linhas horizontais
    for (int y = 0; y < 64; y += 8) {
        ssd1306_draw_line(0, y, 127, y);
    }
    
    ssd1306_update_display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ssd1306_clear_buffer();
    
    // Linhas verticais
    for (int x = 0; x < 128; x += 16) {
        ssd1306_draw_line(x, 0, x, 63);
    }
    
    ssd1306_update_display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ssd1306_clear_buffer();
    
    // Linhas diagonais
    ssd1306_draw_line(0, 0, 127, 63);
    ssd1306_draw_line(0, 63, 127, 0);
    ssd1306_draw_line(64, 0, 64, 63);
    ssd1306_draw_line(0, 32, 127, 32);
    
    ssd1306_update_display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

// Função para testar retângulos
void test_rectangles(void) {
    printf("Teste 4: Retângulos");
    ssd1306_clear_buffer();
    
    // Retângulos vazios
    ssd1306_draw_rect(10, 10, 30, 20, false);
    ssd1306_draw_rect(50, 10, 30, 20, false);
    ssd1306_draw_rect(90, 10, 30, 20, false);
    
    // Retângulos preenchidos
    ssd1306_draw_rect(10, 40, 15, 15, true);
    ssd1306_draw_rect(35, 40, 15, 15, true);
    ssd1306_draw_rect(60, 40, 15, 15, true);
    ssd1306_draw_rect(85, 40, 15, 15, true);
    
    ssd1306_update_display();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

// Função para animação de barra de progresso
void test_progress_bar(void) {
    printf("Teste 5: Barra de progresso");
    
    for (int progress = 0; progress <= 100; progress += 5) {
        ssd1306_clear_buffer();
        
        // Título
        ssd1306_draw_string(20, 0, "Carregando...");
        
        // Moldura da barra
        ssd1306_draw_rect(10, 25, 108, 14, false);
        
        // Preenchimento da barra
        int fill_width = (progress * 106) / 100;
        if (fill_width > 0) {
            ssd1306_draw_rect(11, 26, fill_width, 12, true);
        }
        
        // PorcenTAG_MAINem
        char percent_str[16];
        snprintf(percent_str, sizeof(percent_str), "%d%%", progress);
        ssd1306_draw_string(50, 50, percent_str);
        
        ssd1306_update_display();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// Função para desenhar círculo simples
void draw_circle(int cx, int cy, int radius) {
    for (int angle = 0; angle < 360; angle += 10) {
        float rad = angle * M_PI / 180.0;
        int x = cx + (int)(radius * cos(rad));
        int y = cy + (int)(radius * sin(rad));
        ssd1306_set_pixel(x, y, true);
    }
}

// Função para teste de formas circulares
void test_circles(void) {
    printf("Teste 6: Círculos");
    ssd1306_clear_buffer();
    
    draw_circle(32, 32, 20);
    draw_circle(96, 32, 15);
    draw_circle(64, 32, 10);
    
    ssd1306_update_display();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

// Função para animação de ondas
void test_wave_animation(void) {
    printf("Teste 7: Animação de ondas");
    
    for (int frame = 0; frame < 50; frame++) {
        ssd1306_clear_buffer();
        
        ssd1306_draw_string(30, 0, "Animacao");
        
        // Desenhar onda senoidal
        for (int x = 0; x < 128; x++) {
            float wave = sin((x + frame * 5) * M_PI / 30.0);
            int y = 32 + (int)(wave * 15);
            ssd1306_set_pixel(x, y, true);
        }
        
        ssd1306_update_display();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Função para exibir informações do sistema
void test_system_info(void) {
    printf("Teste 8: Informações do sistema");
    ssd1306_clear_buffer();
    
    ssd1306_draw_string(0, 0, "ESP32 Info:");
    
    // Tempo de funcionamento
    int64_t uptime = esp_timer_get_time() / 1000000; // segundos
    char uptime_str[32];
    snprintf(uptime_str, sizeof(uptime_str), "Uptime: %llds", uptime);
    ssd1306_draw_string(0, 16, uptime_str);
    
    // Memória livre
    size_t free_heap = esp_get_free_heap_size();
    char heap_str[32];
    snprintf(heap_str, sizeof(heap_str), "Heap: %dB", (int)free_heap);
    ssd1306_draw_string(0, 32, heap_str);
    
    ssd1306_draw_string(0, 48, "Teste OK!");
    
    ssd1306_update_display();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

// Função para teste de rolagem de texto
void test_scrolling_text(void) {
    printf("Teste 9: Texto rolante");
    
    const char* long_text = "Este eh um texto longo que vai rolar pela tela do display OLED SSD1306!";
    int text_len = strlen(long_text);
    
    for (int offset = 128; offset > -(text_len * 8); offset -= 2) {
        ssd1306_clear_buffer();
        
        ssd1306_draw_string(0, 0, "Texto Rolante:");
        ssd1306_draw_line(0, 15, 127, 15);
        
        ssd1306_draw_string(offset, 25, long_text);
        
        ssd1306_update_display();
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Função para teste de contraste
void test_contrast(void) {
    printf("Teste 10: Níveis de contraste");
    
    uint8_t contrast_levels[] = {0x00, 0x7F, 0xFF, 0xCF};
    const char* labels[] = {"Min", "Medio", "Max", "Normal"};
    
    for (int i = 0; i < 4; i++) {
        // Ajustar contraste
        ssd1306_write_command(SSD1306_CMD_SET_CONTRAST);
        ssd1306_write_command(contrast_levels[i]);
        
        ssd1306_clear_buffer();
        ssd1306_draw_string(0, 0, "Teste Contraste:");
        ssd1306_draw_string(0, 20, labels[i]);
        
        // Desenhar padrão de teste
        for (int y = 35; y < 60; y++) {
            for (int x = 0; x < 128; x++) {
                if ((x + y) % 4 == 0) {
                    ssd1306_set_pixel(x, y, true);
                }
            }
        }
        
        ssd1306_update_display();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
