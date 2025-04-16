#pragma once

#include <xf/Queue.hpp>

#include <proto/app/AppCommand.pb.h>

namespace maestro::command {

using Queue = xf::Queue<AppCommand, 5>;

}
