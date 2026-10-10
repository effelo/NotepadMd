// Copyright (C) 2026 NotepadMd contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <string>
#include <vector>
struct MarkdownFoldLine { int depth = 0; bool isHeader = false; };
std::vector<MarkdownFoldLine> ScanMarkdownHeadings(const std::vector<std::string>& lines);
