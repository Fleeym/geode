#pragma once
#include "Types.hpp"
#include "../utils/addresser.hpp"

namespace geode::modifier {
	struct addresses {
		#include <codegen/GeneratedAddress.hpp>
	};
}
