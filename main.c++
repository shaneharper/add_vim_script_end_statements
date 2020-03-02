#include <iostream>
#include <string>
std::string with_end_statements(std::istream&);

#ifdef _WIN32
# include <io.h> // _setmode
# include <fcntl.h>
static void use_newlines_without_linefeeds_in_output()
{
    (void)_setmode(_fileno(stdout), O_BINARY);
    (void)_setmode(_fileno(stderr), O_BINARY);
}
#endif


int main()
try
{
#ifdef _WIN32
    use_newlines_without_linefeeds_in_output();  // Unix-style line endings are used when running on Windows in case the generated script ever needs to be run on Unix; Vim on Unix will throw "E492: Not an editor command: ^M" on reading a Vim script file with Windows-style line endings. (It's ok to use Unix-style line endings on Windows though.)
#endif
    std::cout << with_end_statements(std::cin);
}
catch (const char* const exception)
{
    std::cerr << exception << std::endl;
    return 1;
}
