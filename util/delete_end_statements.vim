g/\v^"?\s*(end\S*|augroup\s*(end|END))$/d           " Delete lines which have an end statement (but no other statements or comments).
%s/\v^"?\s*\zs(end\S*|augroup\s*(end|END))\s*//e    " Delete end statements on lines where the end statement is followed by a comment (or something else).
%s/\s*|\s*end\S*//ce                                " Delete end statements which follow other statements on the same line, e.g. delete "| endif" in "if c | call F() | endif".
