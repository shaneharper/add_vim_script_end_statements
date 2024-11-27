#include <iostream>
#include <regex>
#include <sstream>
#include <string>
using namespace std::string_literals;
std::string with_end_statements(std::istream&);


unsigned failed_tests_count = 0;


void test(const char* test_name, const char* input, const char* expected_output)
{
    const auto output = [&input]()
    {
        try
        {
            std::istringstream is(input);
            return with_end_statements(is);
        }
        catch (const char* const exception)
        {
            return "Exception: "s + exception + '\n';
        }
    }
    ();

    if (output != expected_output)
    {
        ++failed_tests_count;
        std::cerr << "*** " << test_name << " failed. ***\n"
            << "EXPECTED:\n" << expected_output << '\n'
            << "ACTUAL:\n" << output << '\n';
    }
}


void test(const char* name, const char* annotated_script /*lines prefixed with >>> are removed from the input; they are expected in the output*/)
{
    test(name, /*input*/ std::regex_replace(annotated_script, std::regex("\n>>>.*"), "").c_str(),
         /*expected_output*/ std::regex_replace(annotated_script, std::regex("\n>>>"), "\n").c_str());
}


int main()
{
    test("function, if, while, for",
         "function X()\n"
         "  if 0\n"
         "    echo\n"
      ">>>  endif\n"
      ">>>endfunction\n"
         "\n"
         "\n"               // (It looks nicer with the "endif" and "endfunction" appearing before the blank lines rather than after them.)
         "function! X()\n"
         "  while 1\n"
         "    for i in [1,2,3]\n"
         "      echo i\n"
      ">>>    endfor\n"       // Note previous "end statements" were output due to a decrease in statement indentation. These "end statements" though are output because there are no more statements.
      ">>>  endwhile\n"
      ">>>endfunction\n"
         "\n"
         "\" vim:sw=4\n"    // It's nice for the modeline to remain the last line.
         );

    test("if statement and its body on same line",
         "if 1 | echo\n"
      ">>>endif\n");

    test("else, elseif",
         "if 0\n"
         "  echo\n"
         "else\n"           // (If this wasn't an "else" (or "elseif") an "endif" would have to be output first because it's not indented like the "if 0" above.)
         "  echo\n"
         "  while 1\n"
         "    echo\n"
      ">>>  endwhile\n"     // Note on reaching the "else" below we need to output end statements such as this, but not for the "if" that is not indented.
         "elseif 0\n"
         " echo\n"
      ">>>endif\n");

    test("if statement extra tests",
         "echo 'if'\n"      // No endif output for this line containing "if".
         "if 1\n"
         "  if 1\n"         // nested if.
         "    echo\n"
      ">>>  endif\n"
         "else\n"
         "  echo\n"
      ">>>endif\n");

    test("try, catch, finally",
         "try\n"
         "  echo\n"
         "catch /1/\n"
         "  echo\n"
         "finally\n"
         "  echo\n"
      ">>>endtry\n"
         "\n"
         "try\n"
         " echo\n"
      ">>>endtry\n");

    test("augroup",
         "augroup X\n"
         "  autocmd!\n"
         "  autocmd BufWritePost ~/.vimrc  so ~/.vimrc\n"
      ">>>augroup end\n");

    test("vim9script def",
         "vim9script\n"
         "\n"
         "def EchoHi()\n"
         "  echo 'Hi'\n"
      ">>>enddef\n"
         "\n"
         "EchoHi()\n");

    test("DOS line endings",
         "insert!\r\n"
         "text\r\n"
         ".\r\n"            // << Initially this was not recognised as the end of the insert statement and consequently no "endif" was added as "if 1" was considered part of the input for :insert. (".\r" != ".")
         "if 1\r\n"
         "\r\n"             // <<< This initially caused the test to fail. ("\r" had been interpreted like a statement in the first column.)
         "  echo\r\n"
         "echo",
         // Note: '\r's don't appear in the output. Ideally, perhaps, line endings in the output should match those in the input. Using '\n' instead of '\r\n' shouldn't be a problem though; it should be possible to run the the resulting script on either Windows or Linux. (With '\r's Vim on Linux could throw "E492: Not an editor command: ^M".) See :help :source_crnl.
         "insert!\n"
         "text\n"
         ".\n"
         "if 1\n"
         "\n"
         "  echo\n"
         "endif\n"
         "echo\n" /*The last line ends with '\n' even though the source didn't end with '\r\n'. (It is common for the last line of a text file to end with \n' on Unix.)*/);

#if 0  // xxx We only look for statements that prefix a new code block (or heredoc) at the start of a line.
    test("multiple statements on one-line",
         "let c = Cond() | if c\n"
         "  echo\n"
      ">>>endif\n");
#endif

#if 0
   test("comment at end of function",
        "function X()\n"
        "  echo\n"
        "\n"
        "  \" comment\n"    // This comment has the same number of spaces of indentation as the last statement in the function. It probably makes the most sense for the "endfunction" to be added after this comment.
     ">>>endfunction\n"
        "\n"
        "echo\n");
#endif


    // Line-continuation -----------------------------------------------------------------

    test("line continuation and line continuation comment",
         "if\n"
         // (Check no "endif" inserted here even though the next line is not indented.)
         "\"\\ Comment.\n"
         "\\ 1\n"
         "  let a =\n"
         "\\       42\n"
      ">>>endif\n");

    test("Unexpected line continuation symbol at start of file",
         "\\if 1\n echo\n",
         "Exception: Unexpected line continuation symbol.\n" /*xxx "on line 1".*/);

    test("Unexpected line continuation symbol following blank line",
         "if\n\n\\ 1\n echo\n",
         "Exception: Unexpected line continuation symbol.\n" /*xxx "on line 3".*/);


    // Heredocs --------------------------------------------------------------------------
    // Text within a heredoc must be left as is.

    test("insert and append",
         "function X()\n"
         "\n"
         "  insert\n"
         "\n"
         "    for\n"        // No "endfor" to be inserted.
         "\n"
         ".\n"
      ">>>endfunction\n"
         "append\n"
         "function Y()\n"   // No "endfunction" inserted.
         // Vim interprets EOF as a valid end of a heredoc.
         );

#if 0
    test("insert and append with a location prefix",
         "5insert\n"
         "if 0\n"
         ".\n"
         "/needle/append\n"
         "Here it is!\n"
         ".\n");            // xxx test with other locations, e.g. $, 'm, etc. (:help range lists all options.)
#endif

    test("pythonx heredoc",
         "pythonx <<\n"
         "if 1:\n"
         " def the_answer():\n"
         "  return 42\n"
         ".\n"
         "\n"
         "pythonx print(\"Hi\")\n" // Check that what follows isn't treated as a heredoc.
         "if 0\n"
         " echo\n"
         ">>>endif\n");

#if 0
    test("pythonx heredoc: intro line is split over two lines",
         "pythonx\n"
         "  \\  <<\n"       // Vim accepts this but I don't think it's worth the trouble to get this test to pass.
         "if 1:\n"
         " pass\n"
        );
#endif

    test("python heredoc with custom end marker",
         "python << ?/EOF!\n"
         "if 1:\n"
         " def the_answer():\n"
         "  return 42\n"
         "?/EOF!\n"
         "\n"
         "python3 << ?/EOF!\n"
         "if 1:\n"
         " pass\n"
         "?/EOF!\n");

    test("lua, perl, ruby, mzscheme and tcl heredocs",
         "lua <<\nif 1\n.\n"
         "perl <<\nif 1\n.\n"
         "ruby <<\nif 1\n.\n"
         "mzscheme <<\nif 1\n.\n"
         "tcl <<\nif 1\n.\n");

    test("let and const heredocs",
         "let text =<< trim END\n"  // (Note: "trim" is optional. end_marker is required.)
         "  if 1\n"
         "END\n"
         "let text =<<XXX\n"  // ("trim" not specified.)
         "if 1\n"
         "XXX\n"
         "\n"
         "if 1\n"
         "  const k =<< trim END\n"
         "    if no_endif_required\n"
         "END\n"
         "  cons k2 =<< END\n"
         "    if no_endif_required\n"
         "END\n"
      ">>>endif\n");

#if 0
    test("Invalid let heredoc",
         "let text =<<\n  if 1\n.\n",
         "Exception: Invalid let heredoc - end marker not specified.\n");
#endif

#if 0  // ignore...
    test("heredoc beginning with \\",  // see :h line-continuation
         "set cpo+=C\n"
         "append\n"
         "\asdf\n"          // we don't want this and the previous line to be interpreted as "appendasdf" (and then "append" not to be recognised as introducing a heredoc).
         "function F()\n"   // no "endfunction" required.
         ".\n"
         ":set cpo-=C\n");
#endif

    if (!failed_tests_count) { std::cout << "Ok.\n"; }
    return failed_tests_count;
}
