#include <maestro/Maestro.hpp>
#include <util/log.hpp>

extern "C" void app_main() {
    try {
        maestro::Maestro::start();
    } catch (const etk::error::Exception& etk_exception) {
        LOGE("Main", "ESP-IDF exception on start (ec={}, name={})", etk_exception.error(), etk_exception.what());
    } catch (const std::exception& exception) {
        LOGE("Main", "General exception on start (what={})", exception.what());
    }
}
