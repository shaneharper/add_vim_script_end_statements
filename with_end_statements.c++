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

    std::string comment_and_blank_lines_following_last_statement;  // Lines are appended to this until the next line with a statement is reached (or EOF). This is used so that, for example, "endfunction" can be output immediately after the last statement in a function rather than after any blank lines that follow the function. Note: not until we've read the first non-blank/non-comment line after the end of a function can we determine from the indentation that the function definition finished.

    const auto add_end_statements = [&](unsigned indent)
    {
        const auto spaces = [](unsigned num) { return std::string(num, ' '); };
        const auto end_statement = [](const std::string& keyword) -> std::string
        {
            return keyword == "augroup" ? "augroup end" : "end"s + keyword;
        };
        for (; not active_code_blocks.empty() and indent <= active_code_blocks.top().indent; active_code_blocks.pop())
        {
            r += spaces(active_code_blocks.top().indent) + end_statement(active_code_blocks.top().keyword) + '\n';
        }
    };

    const auto copy_heredoc = [&](const std::string& end_marker)
    {
        for (std::string line; getline(is, line);)
        {
            r += line + '\n';
            if (end_marker == line) break;
        }
        // Note: Vim doesn't seem to have a problem if EOF is reached without there being a line with '.' after :insert, :append, :python3 or :pythonx.
    };

    for (std::string line; getline(is, line);)
    {
        if (line.length() and *(line.cend()-1) == '\r') { line.pop_back(); }

        const auto indent = line.find_first_not_of(' ');  // xxx This doesn't consider tabs.
        if (indent == line.npos) { comment_and_blank_lines_following_last_statement += '\n'; }
        else if (line[indent] == '\\' /*line continuation character*/
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

            static const std::regex ELSE_ELSEIF_CATCH_OR_FINALLY_RE("^(else|elseif|catch|finally)\\b");
            add_end_statements(indent + (matches(ELSE_ELSEIF_CATCH_OR_FINALLY_RE) ? 1: 0));
            r += comment_and_blank_lines_following_last_statement + line + '\n';
            comment_and_blank_lines_following_last_statement = "";

            if (static const std::regex RE("^(augroup|def|function|if|for|try|while)\\b");  // xxx doesn't check for abbreviations, e.g. func for function.
                const auto m = matches(RE))
            {
                active_code_blocks.push({unsigned(indent), m.str(1)});
            }
            else if (static const std::regex RE("^(insert|append)\\b"); matches(RE))
            {
                copy_heredoc(".");
            }
            else if (static const std::regex RE(R"_(^(lua|mzscheme|perl|python|python[3x]|ruby|tcl)\s*<<\s*(\S*))_");  // xxx Match pyx, py3, py, and other abbreviations.
                     const auto m = matches(RE))
            {
                copy_heredoc(m.str(2).empty() ? "." : m.str(2));
            }
            else if (static const std::regex RE(R"_(^(let|const)\s*\w+\s*=<<\s*(trim|)\s*([^a-z ]\S*))_");
                     const auto m = matches(RE))
            {
                copy_heredoc(m.str(3));
            }
        }
    }
    add_end_statements(0);
    return r + comment_and_blank_lines_following_last_statement;
}
