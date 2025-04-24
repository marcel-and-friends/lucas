#pragma once

#include <esp_log.h>
#include <format>

#define LOGI(tag, fmt, ...)                                           \
    do {                                                              \
        auto string = std::format(fmt, ##__VA_ARGS__);                \
        ESP_LOGI("Lucas/" tag, "%*s", string.size(), string.c_str()); \
    } while (false)

#define LOGW(tag, fmt, ...)                                           \
    do {                                                              \
        auto string = std::format(fmt, ##__VA_ARGS__);                \
        ESP_LOGW("Lucas/" tag, "%*s", string.size(), string.c_str()); \
    } while (false)

#define LOGE(tag, fmt, ...)                                           \
    do {                                                              \
        auto string = std::format(fmt, ##__VA_ARGS__);                \
        ESP_LOGE("Lucas/" tag, "%*s", string.size(), string.c_str()); \
    } while (false)
