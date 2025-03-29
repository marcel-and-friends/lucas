#include <print>

#include "Maestro.hpp"
#include <etk/nvs/PersistentValue.hpp>
#include <proto/firmware/FirmwareEvent.pb.h>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace maestro {

Maestro::Maestro()
    : m_bridge(0) {
    LOGI("Maestro", "Hello");
}

void Maestro::setup_impl() {
}

void Maestro::run_impl() {
    every(10s, [this, counter = 0.0f] mutable {
        m_bridge.send_event({
            .which_event = FirmwareEvent_temperature_report_tag,
            .event = {
                .temperature_report = {
                    .celsius = counter += 100.0f,
                },
            },

        });
        return xf::util::ControlFlow::Continue;
    });
}

}
