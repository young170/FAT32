#include "util.h"
#include <cstdint>

namespace io {
	int to_le1(char* bytes)
	{
		return *bytes;
	}

	int to_le2(char* bytes)
	{
		return *(int16_t *) bytes;
	}

	int to_le4(char* bytes)
	{
		return *(int32_t *) bytes;
	}
};