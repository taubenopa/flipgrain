#include "ClipboardWin.h"

#if defined (_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>

namespace fg
{
std::vector<std::wstring> getClipboardFilePaths()
{
    std::vector<std::wstring> out;
    if (! OpenClipboard (nullptr)) return out;

    if (HANDLE h = GetClipboardData (CF_HDROP))
    {
        HDROP drop = (HDROP) h;
        const UINT count = DragQueryFileW (drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i)
        {
            const UINT len = DragQueryFileW (drop, i, nullptr, 0);
            std::vector<wchar_t> buf ((size_t) len + 1, L'\0');
            DragQueryFileW (drop, i, buf.data(), len + 1);
            out.emplace_back (buf.data());
        }
    }
    CloseClipboard();
    return out;
}
}
#else
namespace fg { std::vector<std::wstring> getClipboardFilePaths() { return {}; } }
#endif
