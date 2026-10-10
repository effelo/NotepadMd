# Third-party notices

NotepadMd builds against four unmodified upstream API headers. The preparation script fetches the official Notepad++ plugin template at commit 27b7077ba89766b3a5a136a3d71af3a0cccc7d2a into ignored .local/plugintemplate. No upstream header is committed to this repository.

## Notepad++ plugin API

| Files | Copyright and license | Pinned source |
| --- | --- | --- |
| PluginInterface.h, Notepad_plus_msgs.h | Copyright (C) 2025 Don HO. Each file states GNU GPL version 3 or later. | [PluginInterface.h](https://github.com/npp-plugins/plugintemplate/blob/27b7077ba89766b3a5a136a3d71af3a0cccc7d2a/src/PluginInterface.h), [Notepad_plus_msgs.h](https://github.com/npp-plugins/plugintemplate/blob/27b7077ba89766b3a5a136a3d71af3a0cccc7d2a/src/Notepad_plus_msgs.h) |

The full GNU GPL version 3 text is in this repository's LICENSE file. The upstream file headers permit use under version 3 or any later version.

## Scintilla API

| Files | Copyright and license | Pinned source |
| --- | --- | --- |
| Scintilla.h, Sci_Position.h | Copyright Neil Hodgson. Scintilla license, reproduced below. | [Scintilla.h](https://github.com/npp-plugins/plugintemplate/blob/27b7077ba89766b3a5a136a3d71af3a0cccc7d2a/src/Scintilla.h), [Sci_Position.h](https://github.com/npp-plugins/plugintemplate/blob/27b7077ba89766b3a5a136a3d71af3a0cccc7d2a/src/Sci_Position.h) |

The license text below comes from [Scintilla's License.txt](https://www.scintilla.org/License.txt). Keep it with any source or binary release that includes these headers or Scintilla material.

### Scintilla license text

    License for Lexilla, Scintilla, and SciTE

    Copyright 1998-2021 by Neil Hodgson <neilh@scintilla.org>

    All Rights Reserved

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appear in all copies and that
    both that copyright notice and this permission notice appear in
    supporting documentation.

    NEIL HODGSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
    SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS, IN NO EVENT SHALL NEIL HODGSON BE LIABLE FOR ANY
    SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
    WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
    OR PERFORMANCE OF THIS SOFTWARE.
