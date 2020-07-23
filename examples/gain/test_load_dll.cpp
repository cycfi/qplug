#include <Windows.h>
#include <filesystem>

int main()
{
   // wchar_t* path = L"..\\gain.vst3\\Contents\\x86_64-win\\gain.vst3";

   std::filesystem::path path = L"gain.vst3";
   auto dll_path = path / "Contents" / "x86_64-win" / "gain.vst3";

   auto exists = std::filesystem::exists(path);
   auto exists2 = std::filesystem::exists(dll_path);

   auto wstr = dll_path.wstring();
   auto module = LoadLibraryW(wstr.data());

   auto err = GetLastError();

   return module == nullptr;
}

