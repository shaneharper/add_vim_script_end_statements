#include <iostream>
#include <string>
std::string with_end_statements(std::istream&);


int main()
try
{
    std::cout << with_end_statements(std::cin);
}
catch (const char* const exception)
{
    std::cerr << exception << std::endl;
    return 1;
}
