#pragma once

#include <xf/queue/StaticQueue.hpp>

#include <proto/app/HeaterControl.pb.h>

namespace heater::command {

using Queue = xf::queue::StaticQueue<HeaterControl, 2>;

}
