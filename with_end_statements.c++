#include <istream>
#include <regex>
#include <stack>
#include <string>
using namespace std::string_literals;


std::string with_end_statements(std::istream& is)
{
    std::string r;

    struct CodeBlockPrefix
    {
        unsigned indent;
        std::string keyword;
    };
    std::stack<CodeBlockPrefix> active_code_blocks;

    const auto add_end_statements = [&](unsigned indent)
    {
        const auto spaces = [](unsigned num) { return std::string(num, ' '); };
        const auto end_statement = [](const std::string& keyword) -> std::string
        {
            return keyword == "augroup" ? "augroup end" : "end"s + keyword;
        };

        for (;
             not active_code_blocks.empty() and indent <= active_code_blocks.top().indent;
             active_code_blocks.pop())
        {
            r += spaces(active_code_blocks.top().indent) + end_statement(active_code_blocks.top().keyword) + '\n';
        }
    };

    const auto get_line_without_end_of_line_chars = [&](std::string& line)  // The end of line character/s are removed from the stream by this function.
    {
        if (!getline(is, line)) { return false; }
        if (line.ends_with('\r')) { line.pop_back(); }
        return true;
    };

    const auto copy_heredoc = [&](const std::string& end_marker)
    {
        for (std::string line; get_line_without_end_of_line_chars(line);)
        {
            r += line + '\n';
            if (end_marker == line) break;
        }
        // Note: Vim doesn't seem to have a problem if EOF is reached without there being a line with '.' after :insert, :append, :python3 or :pythonx.
    };

    std::string comment_and_blank_lines_following_last_statement;  // Lines are appended to this until the next line with a statement is reached (or there is no more input). This is used so that, for example, "endfunction" can be output immediately after the last statement in a function rather than after any blank lines that follow the function. Note: not until we've read the first non-blank/non-comment line after the end of a function can we determine from the indentation that the function definition finished.

    // xxx Lines are output with just a terminating newline character even if '\r\n' was used in the input. Ideally, perhaps, '\r's should appear in the output if they appeared in the input. (Removing '\r's shouldn't be a problem though - see :help :source_crnl.)
    for (std::string line; get_line_without_end_of_line_chars(line);)
    {
        const auto indent = line.find_first_not_of(' ');  // xxx This doesn't consider tabs.
        if (indent == line.npos)
        {
            comment_and_blank_lines_following_last_statement += '\n';
        }
        else if ('\\' /*line continuation character*/ == line[indent]
                 or 0 == line.compare(indent, 2, "\"\\") /*is a "line continuation comment".*/)
        {
            if (r.empty() or not comment_and_blank_lines_following_last_statement.empty()) { throw "Unexpected line continuation symbol."; }
            r += line + '\n';
        }
        else if ('"' == line[indent])
        {
            comment_and_blank_lines_following_last_statement += line + '\n';
        }
        else
        {
            const auto matches = [&](const std::regex& re)
            {
                struct Matches : std::smatch
                {
                    operator bool() const { return not empty(); }
                } m;
                (void)std::regex_search(line.cbegin() + indent, line.cend(), m, re);
                return m;
            };

            static const std::regex CATCH_ELSE_ELSEIF_OR_FINALLY_RE("^(catch|else|elseif|finally)\\b");
            add_end_statements(indent + (matches(CATCH_ELSE_ELSEIF_OR_FINALLY_RE) ? 1: 0));
            r += comment_and_blank_lines_following_last_statement + line + '\n';
            comment_and_blank_lines_following_last_statement = "";

            if (static const std::regex RE("^(augroup|def|for|function|if|try|while)\\b");  // xxx doesn't check for abbreviations, e.g. func for function.
                const auto m = matches(RE))
            {
                active_code_blocks.push({unsigned(indent), m.str(1)});
            }
            else if (static const std::regex RE("^\\d*(append|insert)\\b"); matches(RE))  // xxx See "insert and append with a location prefix" test.
            {
                copy_heredoc(".");
            }
            else if (static const std::regex RE(R"_(^(lua|mzscheme|perl|python[3x]?|ruby|tcl)\s*<<\s*(\S*))_");  // xxx Match pyx, py3, py, and other abbreviations.
                     const auto m = matches(RE))
            {
                copy_heredoc(m.str(2).empty() ? "." : m.str(2));
            }
            else if (static const std::regex RE(R"_(^(const?|let)\s*\w+\s*=<<\s*(trim|)\s*([^a-z ]\S*))_");
                     const auto m = matches(RE))
            {
                copy_heredoc(m.str(3));
            }
        }
    }
    add_end_statements(0);
    return r + comment_and_blank_lines_following_last_statement;
}
