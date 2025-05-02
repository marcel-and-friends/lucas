#pragma once

#include <xf/queue/StaticQueue.hpp>

#include <proto/app/AppCommand.pb.h>

namespace maestro::command {

using Queue = xf::queue::StaticQueue<AppCommand, 5>;

}
