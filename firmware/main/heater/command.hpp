#pragma once

#include <xf/Queue.hpp>

#include <proto/app/HeaterControl.pb.h>

namespace heater::command {

using Queue = xf::Queue<HeaterControl, 2>;

}
