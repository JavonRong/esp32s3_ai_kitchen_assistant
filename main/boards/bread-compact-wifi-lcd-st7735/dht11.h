#include "mcp_server.h"
#include <esp_log.h>

#define TAG "温湿度事件："

#include "driver/gpio.h"
#include "esp_timer.h"      // 用于获取高精度时间
#include "rom/ets_sys.h"    // 用于 ets_delay_us



class DHT11Controller {
private:
    gpio_num_t gpio_num_;
    int64_t last_read_time_us_ = 0;   // 上次读取时间（微秒）
    float last_temperature_ = 0.0f;   // 缓存的上次温度
    float last_humidity_ = 0.0f;      // 缓存的上次湿度

    // 核心读取函数，返回 true 表示成功，数据通过引用返回
    bool readDHT11(float &temperature, float &humidity) {
        uint8_t data[5] = {0, 0, 0, 0, 0};

        // 1. 发送开始信号（输出模式）
        gpio_set_direction(gpio_num_, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio_num_, 0);          // 拉低
        ets_delay_us(20 * 1000);               // 至少 18ms，这里用 20ms
        gpio_set_level(gpio_num_, 1);           // 拉高
        ets_delay_us(30);                       // 等待 20-40us

        // 2. 切换为输入模式，并等待 DHT11 响应
        gpio_set_direction(gpio_num_, GPIO_MODE_INPUT);
        // 等待 DHT11 拉低总线（响应信号）
        int timeout = 0;
        while (gpio_get_level(gpio_num_) == 1) {
            if (++timeout > 100) return false;  // 超时，无响应
            ets_delay_us(1);
        }
        // 等待 DHT11 拉高（80us 低电平后的 80us 高电平）
        timeout = 0;
        while (gpio_get_level(gpio_num_) == 0) {
            if (++timeout > 100) return false;
            ets_delay_us(1);
        }
        timeout = 0;
        while (gpio_get_level(gpio_num_) == 1) {
            if (++timeout > 100) return false;
            ets_delay_us(1);
        }

        // 3. 读取 40 位数据
        for (int i = 0; i < 40; i++) {
            // 等待低电平开始（每个位开始）
            while (gpio_get_level(gpio_num_) == 0);
            // 延时 30-40us，然后采样
            ets_delay_us(35);
            if (gpio_get_level(gpio_num_) == 1) {
                data[i / 8] = (data[i / 8] << 1) | 1;  // 该位为 1
                // 等待剩余高电平结束
                while (gpio_get_level(gpio_num_) == 1);
            } else {
                data[i / 8] = (data[i / 8] << 1) | 0;  // 该位为 0
            }
        }

        // 4. 校验（可选，DHT11 最后一位是校验和）
        if ((data[0] + data[1] + data[2] + data[3]) != data[4]) {
            return false;  // 校验失败
        }

        // 5. 解析数据
        humidity = (float)data[0] + (float)data[1] / 10.0f;
        temperature = (float)data[2] + (float)data[3] / 10.0f;
        // 处理负数温度（如果 data[2] 最高位为 1 表示负温度，这里简化处理）
        if (data[2] & 0x80) {
            temperature = -temperature;
        }
        return true;
    }

public:
    DHT11Controller(gpio_num_t gpio_num) : gpio_num_(gpio_num) {
        // 初始化 GPIO 为输出模式，并保持高电平（空闲状态）
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << gpio_num_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&config));
        gpio_set_level(gpio_num_, 1);  // 初始高电平

        auto& mcp_server = McpServer::GetInstance();
        mcp_server.AddTool("self.dht11.read", 
                           "read indoor temperature and humidity from dht11 sensor", 
                           PropertyList(), 
                           [this](const PropertyList& properties) -> ReturnValue {
            // 检查读取间隔（DHT11 至少需要 1 秒间隔）
            int64_t now = esp_timer_get_time();
            if (now - last_read_time_us_ < 1000000) {  // 不足 1 秒
                // 返回上次缓存的值（或可以选择等待，但建议快速返回）
                // 这里返回上次的值并附加一个提示
                char json[128];

                ESP_LOGW(TAG, "{\"temperature\":%.1f,\"humidity\":%.1f,\"cached\":true}", 
                         last_temperature_, last_humidity_);
                return json;
            }

            float temp, hum;
            if (readDHT11(temp, hum)) {
                last_temperature_ = temp;
                last_humidity_ = hum;
                last_read_time_us_ = now;

                char json[128];
                ESP_LOGW(TAG, "{\"temperature\":%.1f,\"humidity\":%.1f,\"cached\":false}", 
                         temp, hum);
                return json;
            } else {
                return "{\"error\":\"Failed to read DHT11\"}";
            }
        });
    }
};