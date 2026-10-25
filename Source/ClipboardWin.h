#pragma once
#include <string>
#include <vector>

namespace fg
{
// Liefert Dateipfade, die im Windows-Explorer kopiert wurden (CF_HDROP). Auf anderen Systemen leer.
std::vector<std::wstring> getClipboardFilePaths();
}
