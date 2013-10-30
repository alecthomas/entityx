
#include <iostream>

int main()
{
	std::cout << "Testing\n";
	std::cout << "Has static_assert: " <<
#ifdef HAS_CXX11_STATIC_ASSERT
		"yes :)"
#else
		"no"
#endif
		<< "\n";
	std::cout << "Has variadic templates: " <<
#ifdef HAS_CXX11_VARIADIC_TEMPLATES
		"yes :)"
#else
		"no"
#endif
		<< "\n";
	return 0;
}

