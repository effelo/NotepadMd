// Copyright (C) 2026 NotepadMd contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MarkdownHeadingScanner.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

void Check(const char* name, const std::vector<std::string>& lines,
    const std::vector<MarkdownFoldLine>& expected)
{
    const auto actual = ScanMarkdownHeadings(lines);
    if (actual.size() != expected.size())
    {
        std::cerr << name << ": wrong line count\n";
        std::exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < actual.size(); ++i)
    {
        if (actual[i].depth != expected[i].depth || actual[i].isHeader != expected[i].isHeader)
        {
            std::cerr << name << ": unexpected fold at line " << i + 1 << "\n";
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    Check("empty", {}, {});
    Check("plain text", {"Text"}, {{0, false}});
    Check("nested ATX",
        {"# Header 1", "Text block 1", "## Header 1.1", "Text block 1.1",
         "### Header 1.1.1", "Text block", "## Header 1.2", "Text block",
         "# Header 2", "Text block"},
        {{0, true}, {1, false}, {1, true}, {2, false}, {2, true},
         {3, false}, {1, true}, {2, false}, {0, true}, {1, false}});
    Check("Setext headings",
        {"Header 1", "========", "Text", "Header 2", "--------", "Text"},
        {{0, true}, {1, false}, {1, false}, {1, true}, {2, false}, {2, false}});
    Check("Setext markers reject internal whitespace",
        {"Text", "= =", "Text", "-\t-", "Text", "-- =", "Text"},
        {{0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}});
    Check("Setext markers allow trailing whitespace",
        {"Header 1", "=== \t", "Text", "Header 2", "---\t ", "Text"},
        {{0, true}, {1, false}, {1, false}, {1, true}, {2, false}, {2, false}});
    Check("fenced code",
        {"# Outside", "```md", "# Inside", "```", "~~~", "## Inside",
         "~~~", "## Outside", "Text"},
        {{0, true}, {1, false}, {1, false}, {1, false}, {1, false},
         {1, false}, {1, false}, {1, true}, {2, false}});
    Check("unsupported containers and invalid headings",
        {"---", "> # Quoted", "- # List", "1. # Numbered",
         "    # Indented", "####### Too deep", "#NoSpace"},
        {{0, false}, {0, false}, {0, false}, {0, false},
         {0, false}, {0, false}, {0, false}});
    Check("heading at end", {"# Last"}, {{0, false}});
    std::cout << "Markdown heading scanner tests passed.\n";
    return EXIT_SUCCESS;
}
