// Copyright (C) 2026 NotepadMd contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MarkdownHeadingScanner.h"

#include <string_view>
#include <vector>

namespace
{
struct Heading
{
    int rank;
    size_t line;
};

struct Fence
{
    char marker = '\0';
    size_t length = 0;
    bool open = false;
};

std::string_view WithoutLineEnding(const std::string& line)
{
    size_t end = line.size();
    while (end && (line[end - 1] == '\r' || line[end - 1] == '\n'))
        --end;
    return { line.data(), end };
}

size_t LeadingSpaces(std::string_view line)
{
    size_t i = 0;
    while (i < line.size() && line[i] == ' ')
        ++i;
    return i;
}

bool IsThematicBreak(std::string_view line)
{
    const size_t first = LeadingSpaces(line);
    if (first > 3 || first == line.size())
        return false;

    const char marker = line[first];
    if (marker != '-' && marker != '*' && marker != '_')
        return false;

    size_t count = 0;
    for (size_t i = first; i < line.size(); ++i)
    {
        if (line[i] == marker)
            ++count;
        else if (line[i] != ' ' && line[i] != '\t')
            return false;
    }
    return count >= 3;
}

bool IsSetextUnderline(std::string_view line, int& rank)
{
    const size_t first = LeadingSpaces(line);
    if (first > 3 || first == line.size())
        return false;

    const char marker = line[first];
    if (marker != '=' && marker != '-')
        return false;

    size_t i = first;
    while (i < line.size() && line[i] == marker)
        ++i;
    for (; i < line.size(); ++i)
        if (line[i] != ' ' && line[i] != '\t')
            return false;

    rank = marker == '=' ? 1 : 2;
    return true;
}

bool IsContainerLine(std::string_view line)
{
    const size_t first = LeadingSpaces(line);
    if (first == line.size())
        return false;

    if (line[first] == '>')
        return true;
    if ((line[first] == '+' || line[first] == '*' || line[first] == '-') &&
        first + 1 < line.size() && (line[first + 1] == ' ' || line[first + 1] == '\t'))
        return true;

    if (line[first] >= '0' && line[first] <= '9')
    {
        size_t i = first;
        while (i < line.size() && line[i] >= '0' && line[i] <= '9')
            ++i;
        if (i < line.size() && (line[i] == '.' || line[i] == ')') &&
            i + 1 < line.size() && (line[i + 1] == ' ' || line[i + 1] == '\t'))
            return true;
    }
    return false;
}

bool ReadFence(std::string_view line, char& marker, size_t& length, bool& closing)
{
    const size_t first = LeadingSpaces(line);
    if (first > 3 || first + 2 >= line.size() ||
        (line[first] != '`' && line[first] != '~'))
        return false;

    marker = line[first];
    size_t i = first;
    while (i < line.size() && line[i] == marker)
        ++i;
    length = i - first;
    if (length < 3)
        return false;

    closing = true;
    for (; i < line.size(); ++i)
    {
        if (line[i] != ' ' && line[i] != '\t')
        {
            closing = false;
            break;
        }
    }
    return true;
}

bool IsAtxHeading(std::string_view line, int& rank)
{
    const size_t first = LeadingSpaces(line);
    if (first > 3 || first == line.size() || line[first] != '#')
        return false;

    size_t i = first;
    while (i < line.size() && line[i] == '#')
        ++i;
    rank = static_cast<int>(i - first);
    return rank <= 6 && (i == line.size() || line[i] == ' ' || line[i] == '\t');
}

bool IsSetextCandidate(std::string_view line)
{
    const size_t first = LeadingSpaces(line);
    if (first == line.size() || first >= 4 || IsThematicBreak(line) || IsContainerLine(line))
        return false;

    int rank = 0;
    if (IsAtxHeading(line, rank))
        return false;

    char marker = '\0';
    size_t length = 0;
    bool closing = false;
    return !ReadFence(line, marker, length, closing);
}

std::vector<Heading> FindHeadings(const std::vector<std::string>& lines)
{
    std::vector<Heading> headings;
    Fence fence;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string_view line = WithoutLineEnding(lines[i]);
        char marker = '\0';
        size_t length = 0;
        bool closing = false;
        if (ReadFence(line, marker, length, closing))
        {
            if (!fence.open)
                fence = { marker, length, true };
            else if (closing && marker == fence.marker && length >= fence.length)
                fence.open = false;
            continue;
        }
        if (fence.open)
            continue;

        int rank = 0;
        if (IsAtxHeading(line, rank))
        {
            headings.push_back({ rank, i });
            continue;
        }
        if (i + 1 < lines.size() && IsSetextCandidate(line) &&
            IsSetextUnderline(WithoutLineEnding(lines[i + 1]), rank))
            headings.push_back({ rank, i });
    }
    return headings;
}
}

std::vector<MarkdownFoldLine> ScanMarkdownHeadings(const std::vector<std::string>& lines)
{
    const std::vector<Heading> headings = FindHeadings(lines);
    std::vector<MarkdownFoldLine> result(lines.size());
    std::vector<int> ranks;
    size_t headingIndex = 0;

    for (size_t line = 0; line < lines.size(); ++line)
    {
        if (headingIndex < headings.size() && headings[headingIndex].line == line)
        {
            const int rank = headings[headingIndex].rank;
            while (!ranks.empty() && ranks.back() >= rank)
                ranks.pop_back();

            result[line].depth = static_cast<int>(ranks.size());
            result[line].isHeader = true;
            ranks.push_back(rank);
            ++headingIndex;
        }
        else
            result[line].depth = static_cast<int>(ranks.size());
    }

    for (size_t line = 0; line < result.size(); ++line)
        if (result[line].isHeader)
            result[line].isHeader = line + 1 < result.size() && result[line + 1].depth > result[line].depth;

    return result;
}
