#include <esp_cpu.h>
#include <esp_rom_sys.h>

#include "error.hpp"

namespace etk::error::detail {

void must_failed_print(esp_err_t error_code, const char* file, int line, const char* function, const char* expression) {
#if CONFIG_ESP_ERR_TO_NAME_LOOKUP
    esp_rom_printf("MUST failed: esp_err_t 0x%x (%s) at 0x%08x\n", error_code, esp_err_to_name(error_code), esp_cpu_get_call_addr(reinterpret_cast<intptr_t>(__builtin_return_address(0))));
#elif
    esp_rom_printf("MUST failed: esp_err_t 0x%x at 0x%08x\n", error_code, esp_cpu_get_call_addr(reinterpret_cast<intptr_t>(__builtin_return_address(0))));
#endif
    esp_rom_printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
}

}
