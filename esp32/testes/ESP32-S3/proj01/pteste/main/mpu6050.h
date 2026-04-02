#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

// Definições do MPU6050
#define MPU6050_ADDR                0x68    // Endereço I2C padrão do MPU6050
#define MPU6050_PWR_MGMT_1          0x6B    // Registro de gerenciamento de energia
#define MPU6050_SMPLRT_DIV          0x19    // Sample Rate Divider
#define MPU6050_CONFIG              0x1A    // Configuração
#define MPU6050_GYRO_CONFIG         0x1B    // Configuração do giroscópio
#define MPU6050_ACCEL_CONFIG        0x1C    // Configuração do acelerômetro
#define MPU6050_WHO_AM_I            0x75    // Registro de identificação

// Registros de dados
#define MPU6050_ACCEL_XOUT_H        0x3B
#define MPU6050_ACCEL_XOUT_L        0x3C
#define MPU6050_ACCEL_YOUT_H        0x3D
#define MPU6050_ACCEL_YOUT_L        0x3E
#define MPU6050_ACCEL_ZOUT_H        0x3F
#define MPU6050_ACCEL_ZOUT_L        0x40
#define MPU6050_TEMP_OUT_H          0x41
#define MPU6050_TEMP_OUT_L          0x42
#define MPU6050_GYRO_XOUT_H         0x43
#define MPU6050_GYRO_XOUT_L         0x44
#define MPU6050_GYRO_YOUT_H         0x45
#define MPU6050_GYRO_YOUT_L         0x46
#define MPU6050_GYRO_ZOUT_H         0x47
#define MPU6050_GYRO_ZOUT_L         0x48

// Configurações I2C
#define I2C_MASTER_SCL_IO           18      // GPIO do SCL
#define I2C_MASTER_SDA_IO           17      // GPIO do SDA
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000  // 100kHz
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

static const char *TAG = "MPU6050";

// Estrutura para armazenar os dados do sensor
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temp;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} mpu6050_data_t;

// Função para inicializar o I2C
esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                             I2C_MASTER_RX_BUF_DISABLE, 
                             I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Função para escrever um byte no MPU6050
esp_err_t mpu6050_write_byte(uint8_t reg_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

// Função para ler um byte do MPU6050
esp_err_t mpu6050_read_byte(uint8_t reg_addr, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

// Função para ler múltiplos bytes do MPU6050
esp_err_t mpu6050_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

// Função para inicializar o MPU6050
esp_err_t mpu6050_init(void)
{
    esp_err_t ret;
    uint8_t who_am_i;
    
    // Verificar se o sensor está conectado
    ret = mpu6050_read_byte(MPU6050_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ler WHO_AM_I: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (who_am_i != 0x68) {
        ESP_LOGE(TAG, "MPU6050 não encontrado. WHO_AM_I = 0x%02X", who_am_i);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "MPU6050 encontrado! WHO_AM_I = 0x%02X", who_am_i);
    
    // Sair do modo sleep
    ret = mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao sair do modo sleep: %s", esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS); // Aguardar estabilização
    
    // Configurar sample rate (1kHz / (1 + 7) = 125Hz)
    ret = mpu6050_write_byte(MPU6050_SMPLRT_DIV, 0x07);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar sample rate: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configurar filtro passa-baixa
    ret = mpu6050_write_byte(MPU6050_CONFIG, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar filtro: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configurar escala do giroscópio (±250°/s)
    ret = mpu6050_write_byte(MPU6050_GYRO_CONFIG, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar giroscópio: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configurar escala do acelerômetro (±2g)
    ret = mpu6050_write_byte(MPU6050_ACCEL_CONFIG, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar acelerômetro: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "MPU6050 inicializado com sucesso!");
    return ESP_OK;
}

// Função para ler todos os dados do sensor
esp_err_t mpu6050_read_all(mpu6050_data_t *data)
{
    uint8_t buffer[14];
    esp_err_t ret = mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, buffer, 14);
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Converter os dados (big-endian para little-endian)
    data->accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    data->accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    data->accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);
    data->temp    = (int16_t)((buffer[6] << 8) | buffer[7]);
    data->gyro_x  = (int16_t)((buffer[8] << 8) | buffer[9]);
    data->gyro_y  = (int16_t)((buffer[10] << 8) | buffer[11]);
    data->gyro_z  = (int16_t)((buffer[12] << 8) | buffer[13]);
    
    return ESP_OK;
}

// Função para converter dados brutos em valores físicos
void mpu6050_convert_data(mpu6050_data_t *raw_data, float *accel_g, float *gyro_dps, float *temp_c)
{
    // Converter aceleração para g (±2g range, 16-bit)
    accel_g[0] = (float)raw_data->accel_x / 16384.0f;
    accel_g[1] = (float)raw_data->accel_y / 16384.0f;
    accel_g[2] = (float)raw_data->accel_z / 16384.0f;
    
    // Converter giroscópio para °/s (±250°/s range, 16-bit)
    gyro_dps[0] = (float)raw_data->gyro_x / 131.0f;
    gyro_dps[1] = (float)raw_data->gyro_y / 131.0f;
    gyro_dps[2] = (float)raw_data->gyro_z / 131.0f;
    
    // Converter temperatura para °C
    *temp_c = (float)raw_data->temp / 340.0f + 36.53f;
}

// Task principal para leitura contínua do sensor
void mpu6050_task(void *pvParameters)
{
    mpu6050_data_t sensor_data;
    float accel_g[3], gyro_dps[3], temp_c;
    
    while (1) {
        esp_err_t ret = mpu6050_read_all(&sensor_data);
        
        if (ret == ESP_OK) {
            // Converter dados brutos para valores físicos
            mpu6050_convert_data(&sensor_data, accel_g, gyro_dps, &temp_c);
            
            // Imprimir os dados
            ESP_LOGI(TAG, "Accel: X=%.2fg Y=%.2fg Z=%.2fg", 
                     accel_g[0], accel_g[1], accel_g[2]);
            ESP_LOGI(TAG, "Gyro: X=%.2f°/s Y=%.2f°/s Z=%.2f°/s", 
                     gyro_dps[0], gyro_dps[1], gyro_dps[2]);
            ESP_LOGI(TAG, "Temp: %.2f°C", temp_c);
            ESP_LOGI(TAG, "---");
        } else {
            ESP_LOGE(TAG, "Erro ao ler dados do sensor: %s", esp_err_to_name(ret));
        }
        
        vTaskDelay(500 / portTICK_PERIOD_MS); // Ler a cada 500ms
    }
}
