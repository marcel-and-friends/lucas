#pragma once

#include <etk/nvs/Store.hpp>
#include <xf/task/task.hpp>

#include "Bridge.hpp"

namespace maestro {

class Maestro : public xf::task::StaticTask<Maestro, 3192> {
    XF_TASK;

public:
    explicit Maestro();

    void setup_impl();

    void run_impl();

private:
    Bridge m_bridge;
};

}
