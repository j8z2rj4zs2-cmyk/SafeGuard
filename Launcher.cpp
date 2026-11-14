#include <Windows.h>
int main() {
    HMODULE m = LoadLibraryA("SafeGuard.dll");
    if (!m) {
        MessageBoxA(NULL, "SafeGuard.dll must be in the same folder as Launcher.exe", "SafeGuard", MB_ICONERROR);
        return 1;
    }
    while (GetModuleHandleA("SafeGuard.dll")) Sleep(500);
    return 0;
}
