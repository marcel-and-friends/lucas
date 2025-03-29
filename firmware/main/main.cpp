#include <maestro/Maestro.hpp>

extern "C" void app_main() {
    static maestro::Maestro g_maestro;
    g_maestro.create("MAESTRO", 10);
}
