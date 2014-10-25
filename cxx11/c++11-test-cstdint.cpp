#include <cstdint>
int main()
{
	bool test = 
		(sizeof(std::int8_t) == 1) &&
		(sizeof(std::int16_t) == 2) &&
		(sizeof(std::int32_t) == 4) &&
		(sizeof(std::int64_t) == 8);
	return test ? 0 : 1;
}
