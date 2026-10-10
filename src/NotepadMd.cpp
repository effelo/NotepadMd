// Copyright (C) 2026 NotepadMd contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginInterface.h"
#include "MarkdownHeadingScanner.h"

#include <algorithm>
#include <cwctype>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
constexpr wchar_t kPluginName[] = L"NotepadMd";
constexpr size_t kPathCapacity = 1024;
constexpr UINT kRefreshDelayMs = 100;
constexpr int kCommandCount = 1;

NppData g_nppData;
FuncItem g_commands[kCommandCount];
UINT_PTR g_refreshTimer = 0;

HWND CurrentScintilla()
{
    if (!g_nppData._nppHandle)
        return nullptr;

    int view = -1;
    ::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&view));
    if (view == 0)
        return g_nppData._scintillaMainHandle;
    if (view == 1)
        return g_nppData._scintillaSecondHandle;
    return nullptr;
}

bool IsMarkdownDocument()
{
    if (!g_nppData._nppHandle)
        return false;

    int language = -1;
    ::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, reinterpret_cast<LPARAM>(&language));
    if (language != L_USER)
        return false;

    wchar_t path[kPathCapacity] = {};
    ::SendMessage(g_nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, reinterpret_cast<LPARAM>(path));
    const std::wstring filename(path);
    const size_t dot = filename.find_last_of(L'.');
    if (dot == std::wstring::npos)
        return false;

    std::wstring extension = filename.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return extension == L".md" || extension == L".markdown";
}

std::string ReadLine(HWND editor, int line)
{
    const LRESULT length = ::SendMessage(editor, SCI_LINELENGTH, line, 0);
    if (length <= 0)
        return {};

    std::vector<char> buffer(static_cast<size_t>(length) + 1, '\0');
    ::SendMessage(editor, SCI_GETLINE, line, reinterpret_cast<LPARAM>(buffer.data()));
    return std::string(buffer.data(), static_cast<size_t>(length));
}

bool IsBlank(const std::string& line)
{
    for (const unsigned char ch : line)
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
            return false;
    return true;
}

void RefreshActiveDocument()
{
    HWND editor = CurrentScintilla();
    if (!editor || !IsMarkdownDocument())
        return;

    const int lineCount = static_cast<int>(::SendMessage(editor, SCI_GETLINECOUNT, 0, 0));
    if (lineCount <= 0)
        return;

    std::vector<std::string> lines;
    lines.reserve(static_cast<size_t>(lineCount));
    std::unordered_map<std::string, std::vector<bool>> previousExpansionByHeading;

    for (int line = 0; line < lineCount; ++line)
    {
        lines.push_back(ReadLine(editor, line));
        const int oldLevel = static_cast<int>(::SendMessage(editor, SCI_GETFOLDLEVEL, line, 0));
        if (oldLevel & SC_FOLDLEVELHEADERFLAG)
        {
            const bool expanded = ::SendMessage(editor, SCI_GETFOLDEXPANDED, line, 0) != 0;
            previousExpansionByHeading[lines.back()].push_back(expanded);
        }
    }

    const std::vector<MarkdownFoldLine> folds = ScanMarkdownHeadings(lines);
    for (int line = 0; line < lineCount; ++line)
    {
        const size_t index = static_cast<size_t>(line);
        int level = SC_FOLDLEVELBASE + folds[index].depth;
        if (folds[index].isHeader)
            level |= SC_FOLDLEVELHEADERFLAG;
        if (IsBlank(lines[index]))
            level |= SC_FOLDLEVELWHITEFLAG;

        const int oldLevel = static_cast<int>(::SendMessage(editor, SCI_GETFOLDLEVEL, line, 0));
        if (oldLevel != level)
            ::SendMessage(editor, SCI_SETFOLDLEVEL, line, level);
    }

    std::unordered_map<std::string, size_t> restoredOccurrences;
    for (int line = 0; line < lineCount; ++line)
    {
        const size_t index = static_cast<size_t>(line);
        if (!folds[index].isHeader)
            continue;

        const std::string& heading = lines[index];
        const auto previous = previousExpansionByHeading.find(heading);
        if (previous == previousExpansionByHeading.end())
            continue;

        size_t& occurrence = restoredOccurrences[heading];
        if (occurrence < previous->second.size() && !previous->second[occurrence])
            ::SendMessage(editor, SCI_SETFOLDEXPANDED, line, FALSE);
        ++occurrence;
    }
}

void CALLBACK RefreshTimerProc(HWND window, UINT, UINT_PTR timerId, DWORD)
{
    ::KillTimer(window, timerId);
    g_refreshTimer = 0;
    RefreshActiveDocument();
}

void ScheduleRefresh()
{
    if (g_refreshTimer)
        ::KillTimer(nullptr, g_refreshTimer);
    g_refreshTimer = ::SetTimer(nullptr, 0, kRefreshDelayMs, RefreshTimerProc);
}

void RecomputeMarkdownFolds()
{
    RefreshActiveDocument();
}

void InitializeCommand()
{
    lstrcpyW(g_commands[0]._itemName, L"Recompute Markdown folds");
    g_commands[0]._pFunc = RecomputeMarkdownFolds;
    g_commands[0]._cmdID = 0;
    g_commands[0]._init2Check = false;
    g_commands[0]._pShKey = nullptr;
}
}

extern "C" __declspec(dllexport) void setInfo(NppData data)
{
    g_nppData = data;
    InitializeCommand();
}

extern "C" __declspec(dllexport) const wchar_t* getName()
{
    return kPluginName;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* count)
{
    *count = kCommandCount;
    return g_commands;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notification)
{
    if (!notification)
        return;

    switch (notification->nmhdr.code)
    {
    case NPPN_READY:
        ::SendMessage(g_nppData._nppHandle, NPPM_ADDSCNMODIFIEDFLAGS, 0, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
        ScheduleRefresh();
        break;

    case NPPN_FILEOPENED:
    case NPPN_FILESAVED:
    case NPPN_BUFFERACTIVATED:
    case NPPN_LANGCHANGED:
    case NPPN_GLOBALMODIFIED:
        ScheduleRefresh();
        break;

    case NPPN_SHUTDOWN:
        if (g_refreshTimer)
            ::KillTimer(nullptr, g_refreshTimer);
        g_refreshTimer = 0;
        break;

    case SCN_MODIFIED:
        if ((notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) != 0 &&
            (notification->modificationType & SC_MOD_CHANGEFOLD) == 0)
            ScheduleRefresh();
        break;

    default:
        break;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM)
{
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
