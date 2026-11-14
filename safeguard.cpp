// SafeGuard.dll - external overlay cheat for Roblox
// reads Roblox memory externally, draws overlay, sends input
// Byfron-safe: no code enters RobloxPlayerBeta.exe

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "psapi.lib")

namespace Off {
    constexpr uintptr_t PTR_FakeDataModel = 0x8f8e968;
    constexpr uintptr_t PTR_VisualEngine  = 0x8a11808;
    namespace FakeDataModel { constexpr uintptr_t RealDataModel = 0x1f8; }
    namespace VisualEngine  { constexpr uintptr_t ViewMatrix = 0x1b0; }
    namespace DataModel     { constexpr uintptr_t Workspace = 0x168; }
    namespace Workspace     { constexpr uintptr_t CurrentCamera = 0x4a0; }
    namespace Inst          { constexpr uintptr_t Name = 0x0, ChildrenStart = 0x70; }
    namespace BasePart      { constexpr uintptr_t Primitive = 0x178; }
    namespace Prim          { constexpr uintptr_t Position = 0xd4; }
    namespace Humanoid      { constexpr uintptr_t Health = 0x184, MaxHealth = 0x1a0; }
}

struct RBX {
    HANDLE proc = nullptr;
    DWORD  pid = 0;
    HWND   hwnd = nullptr;
    uintptr_t base = 0;

    bool attach() {
        hwnd = FindWindowA(nullptr, "Roblox");
        if (!hwnd) return false;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid) return false;
        proc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
        if (!proc) return false;
        HMODULE mods[256]; DWORD cb;
        if (EnumProcessModules(proc, mods, sizeof(mods), &cb)) base = (uintptr_t)mods[0];
        return base != 0;
    }
    void detach() { if (proc) CloseHandle(proc); proc = nullptr; base = 0; hwnd = nullptr; }

    template<typename T> T read(uintptr_t addr) const {
        T v{};
        if (proc && addr) ReadProcessMemory(proc, (LPCVOID)addr, &v, sizeof(T), nullptr);
        return v;
    }
    std::string readStr(uintptr_t addr) const {
        if (!proc || !addr) return "";
        char buf[128]{};
        ReadProcessMemory(proc, (LPCVOID)addr, buf, 127, nullptr);
        uint64_t asPtr = *(uint64_t*)buf;
        if (asPtr > 0x10000 && asPtr < 0x7FFFFFFFFFFFLL) {
            ReadProcessMemory(proc, (LPCVOID)asPtr, buf, 127, nullptr);
        }
        buf[127] = 0;
        return std::string(buf);
    }

    uintptr_t dataModel() {
        uintptr_t fdm = read<uintptr_t>(base + Off::PTR_FakeDataModel);
        return fdm ? read<uintptr_t>(fdm + Off::FakeDataModel::RealDataModel) : 0;
    }
    uintptr_t workspace() {
        auto dm = dataModel();
        return dm ? read<uintptr_t>(dm + Off::DataModel::Workspace) : 0;
    }
    uintptr_t visualEngine() { return read<uintptr_t>(base + Off::PTR_VisualEngine); }
};

struct Cfg {
    std::atomic<bool> running{true};
    std::atomic<bool> menuOpen{true};
    std::atomic<bool> espBox{false}, espHealth{false}, espName{false}, espDistance{false};
    float espMaxDistance = 2000.0f;
    ImVec4 espBoxColor{0.0f, 1.0f, 0.5f, 1.0f};
    ImVec4 espNameColor{1,1,1,1};
    std::atomic<bool> aimlockOn{false};
    bool aimlockHold = true;
    int aimlockKey = VK_RBUTTON;
    float aimlockSmoothX = 4.0f, aimlockSmoothY = 4.0f;
    float aimlockSensitivity = 1.0f;
    float aimlockFov = 180.0f;
    std::atomic<bool> antiAfk{true};
    int antiAfkInterval = 60;
};
Cfg g_cfg;
RBX g_rbx;

struct Vec3 { float x, y, z; };

bool worldToScreen(const Vec3& w, float& sx, float& sy, float sw, float sh) {
    uintptr_t ve = g_rbx.visualEngine();
    if (!ve) return false;
    float m[16]; SIZE_T r = 0;
    ReadProcessMemory(g_rbx.proc, (LPCVOID)(ve + Off::VisualEngine::ViewMatrix), m, sizeof(m), &r);
    if (r != sizeof(m)) return false;
    float cx = w.x*m[0] + w.y*m[4] + w.z*m[8]  + m[12];
    float cy = w.x*m[1] + w.y*m[5] + w.z*m[9]  + m[13];
    float cw = w.x*m[3] + w.y*m[7] + w.z*m[11] + m[15];
    if (cw < 0.1f) return false;
    sx = (cx / cw * 0.5f + 0.5f) * sw;
    sy = (1.0f - (cy / cw * 0.5f + 0.5f)) * sh;
    return true;
}

struct Enemy { uintptr_t hum, hrp; Vec3 pos; float health, maxHealth; std::string name; };

std::vector<Enemy> enumerateEnemies() {
    std::vector<Enemy> out;
    uintptr_t ws = g_rbx.workspace();
    if (!ws) return out;
    uintptr_t cs = g_rbx.read<uintptr_t>(ws + Off::Inst::ChildrenStart);
    uintptr_t ce = g_rbx.read<uintptr_t>(ws + Off::Inst::ChildrenStart + 8);
    for (uintptr_t p = cs; p < ce && (p - cs) < 0x10000; p += 8) {
        uintptr_t inst = g_rbx.read<uintptr_t>(p);
        if (!inst) continue;
        uintptr_t subCs = g_rbx.read<uintptr_t>(inst + Off::Inst::ChildrenStart);
        uintptr_t subCe = g_rbx.read<uintptr_t>(inst + Off::Inst::ChildrenStart + 8);
        Enemy e{};
        for (uintptr_t sp = subCs; sp < subCe && (sp - subCs) < 0x10000; sp += 8) {
            uintptr_t sub = g_rbx.read<uintptr_t>(sp);
            if (!sub) continue;
            std::string sn = g_rbx.readStr(g_rbx.read<uintptr_t>(sub + Off::Inst::Name));
            if (sn == "Humanoid") e.hum = sub;
            else if (sn == "HumanoidRootPart") e.hrp = sub;
        }
        if (!e.hum || !e.hrp) continue;
        e.health = g_rbx.read<float>(e.hum + Off::Humanoid::Health);
        e.maxHealth = g_rbx.read<float>(e.hum + Off::Humanoid::MaxHealth);
        if (e.health <= 0) continue;
        uintptr_t prim = g_rbx.read<uintptr_t>(e.hrp + Off::BasePart::Primitive);
        if (!prim) continue;
        e.pos.x = g_rbx.read<float>(prim + Off::Prim::Position);
        e.pos.y = g_rbx.read<float>(prim + Off::Prim::Position + 4);
        e.pos.z = g_rbx.read<float>(prim + Off::Prim::Position + 8);
        e.name = g_rbx.readStr(g_rbx.read<uintptr_t>(inst + Off::Inst::Name));
        out.push_back(e);
    }
    return out;
}

void Aimlock() {
    if (!g_cfg.aimlockOn.load()) return;
    if (g_cfg.aimlockHold && !(GetAsyncKeyState(g_cfg.aimlockKey) & 0x8000)) return;
    auto en = enumerateEnemies();
    if (en.empty()) return;
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    float cx = sw * 0.5f, cy = sh * 0.5f;
    Vec3 bestPos{}; bool found = false; float bestAng = g_cfg.aimlockFov;
    for (auto& e : en) {
        float x, y;
        if (!worldToScreen(e.pos, x, y, (float)sw, (float)sh)) continue;
        float dx = x - cx, dy = y - cy;
        float a = sqrtf(dx*dx + dy*dy);
        if (a < bestAng) { bestAng = a; bestPos = e.pos; found = true; }
    }
    if (!found) return;
    float tx, ty;
    if (!worldToScreen(bestPos, tx, ty, (float)sw, (float)sh)) return;
    float dx = tx - cx, dy = ty - cy;
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = (LONG)(dx / g_cfg.aimlockSmoothX * g_cfg.aimlockSensitivity);
    in.mi.dy = (LONG)(dy / g_cfg.aimlockSmoothY * g_cfg.aimlockSensitivity);
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(INPUT));
}

DWORD WINAPI AntiAfkThread(LPVOID) {
    while (g_cfg.running.load()) {
        if (g_cfg.antiAfk.load() && g_rbx.hwnd) {
            INPUT in{};
            in.type = INPUT_MOUSE;
            in.mi.dx = 1; in.mi.dy = 0;
            in.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &in, sizeof(INPUT));
        }
        Sleep(g_cfg.antiAfkInterval * 1000);
    }
    return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return 0;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void DrawESP(ImDrawList* dl, int sw, int sh) {
    bool any = g_cfg.espBox || g_cfg.espHealth || g_cfg.espName || g_cfg.espDistance;
    if (!any) return;
    auto en = enumerateEnemies();
    for (auto& e : en) {
        Vec3 head{ e.pos.x, e.pos.y + 1.8f, e.pos.z };
        Vec3 foot{ e.pos.x, e.pos.y - 2.5f, e.pos.z };
        float hx, hy, fx, fy;
        if (!worldToScreen(head, hx, hy, (float)sw, (float)sh)) continue;
        if (!worldToScreen(foot, fx, fy, (float)sw, (float)sh)) continue;
        float h = fabsf(fy - hy), w = h * 0.5f;
        float bx = (hx + fx) * 0.5f - w * 0.5f, by = hy;
        ImU32 col = ImGui::ColorConvertFloat4ToU32(g_cfg.espBoxColor);
        ImU32 nc = ImGui::ColorConvertFloat4ToU32(g_cfg.espNameColor);
        if (g_cfg.espBox) dl->AddRect(ImVec2(bx, by), ImVec2(bx + w, by + h), col, 0, 0, 1.5f);
        if (g_cfg.espHealth) {
            float t = e.maxHealth > 0 ? e.health / e.maxHealth : 1.0f;
            if (t < 0) t = 0; if (t > 1) t = 1;
            dl->AddRectFilled(ImVec2(bx - 6, by), ImVec2(bx - 2, by + h), IM_COL32(0,0,0,200));
            dl->AddRectFilled(ImVec2(bx - 6, by + h * (1 - t)), ImVec2(bx - 2, by + h),
                IM_COL32((int)(255*(1-t)), (int)(255*t), 0, 255));
        }
        if (g_cfg.espName && !e.name.empty()) {
            dl->AddText(ImVec2(bx + w * 0.5f - (float)e.name.size() * 3, by - 14), nc, e.name.c_str());
        }
        if (g_cfg.espDistance) {
            char buf[32]; sprintf_s(buf, "%.0f", sqrtf(e.pos.x*e.pos.x + e.pos.y*e.pos.y + e.pos.z*e.pos.z));
            dl->AddText(ImVec2(bx + w * 0.5f - 10, by + h + 2), nc, buf);
        }
    }
}

void RenderMenu() {
    if (!g_cfg.menuOpen.load()) return;
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_FirstUseEver);
    bool open = g_cfg.menuOpen.load();
    ImGui::Begin("SafeGuard", &open);
    g_cfg.menuOpen.store(open);
    if (ImGui::BeginTabBar("t")) {
        if (ImGui::BeginTabItem("Combat")) {
            bool a = g_cfg.aimlockOn.load();
            if (ImGui::Checkbox("Aimlock", &a)) g_cfg.aimlockOn.store(a);
            ImGui::Checkbox("Hold to activate", &g_cfg.aimlockHold);
            ImGui::SliderFloat("Smooth X", &g_cfg.aimlockSmoothX, 1.0f, 20.0f);
            ImGui::SliderFloat("Smooth Y", &g_cfg.aimlockSmoothY, 1.0f, 20.0f);
            ImGui::SliderFloat("Sensitivity", &g_cfg.aimlockSensitivity, 0.1f, 5.0f);
            ImGui::SliderFloat("FOV (px)", &g_cfg.aimlockFov, 10.0f, 500.0f);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            bool b;
            b = g_cfg.espBox; if (ImGui::Checkbox("Box", &b)) g_cfg.espBox = b;
            b = g_cfg.espHealth; if (ImGui::Checkbox("Health bar", &b)) g_cfg.espHealth = b;
            b = g_cfg.espName; if (ImGui::Checkbox("Name", &b)) g_cfg.espName = b;
            b = g_cfg.espDistance; if (ImGui::Checkbox("Distance", &b)) g_cfg.espDistance = b;
            ImGui::ColorEdit4("Box color", (float*)&g_cfg.espBoxColor);
            ImGui::ColorEdit4("Name color", (float*)&g_cfg.espNameColor);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            bool b = g_cfg.antiAfk.load();
            if (ImGui::Checkbox("Anti-AFK", &b)) g_cfg.antiAfk.store(b);
            ImGui::SliderInt("Interval (sec)", &g_cfg.antiAfkInterval, 10, 300);
            ImGui::Separator();
            ImGui::Text("Roblox PID: %lu", g_rbx.pid);
            if (ImGui::Button("Re-attach")) { g_rbx.detach(); g_rbx.attach(); }
            if (ImGui::Button("Unload SafeGuard")) g_cfg.running.store(false);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

DWORD WINAPI OverlayThread(LPVOID module) {
    while (g_cfg.running.load() && !g_rbx.attach()) Sleep(1000);
    if (!g_cfg.running.load()) return 0;

    WNDCLASSEXA wc{ sizeof(wc), CS_HREDRAW | CS_VREDRAW, OverlayProc, 0, 0,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "SafeGuardOverlay", NULL };
    RegisterClassExA(&wc);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "SafeGuardOverlay", "SG", WS_POPUP, 0, 0, sw, sh, NULL, NULL, wc.hInstance, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_COLORKEY);
    MARGINS m{ -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &m);
    ShowWindow(hwnd, SW_SHOW);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Width = sw; sd.BufferDesc.Height = sh;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* sc = nullptr;
    D3D_FEATURE_LEVEL fl;
    if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx))) return 1;

    ID3D11Texture2D* bb = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    ID3D11RenderTargetView* rtv = nullptr;
    dev->CreateRenderTargetView(bb, NULL, &rtv);
    bb->Release();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 6.0f;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev, ctx);

    CreateThread(NULL, 0, AntiAfkThread, NULL, 0, NULL);

    MSG msg{};
    while (g_cfg.running.load()) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_cfg.running.store(false);
        }
        static bool latch = false;
        bool kd = (GetAsyncKeyState('9') & 0x8000) != 0;
        if (kd && !latch) {
            g_cfg.menuOpen.store(!g_cfg.menuOpen.load());
            LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
            if (g_cfg.menuOpen.load()) ex &= ~WS_EX_TRANSPARENT;
            else ex |= WS_EX_TRANSPARENT;
            SetWindowLong(hwnd, GWL_EXSTYLE, ex);
        }
        latch = kd;

        Aimlock();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        DrawESP(ImGui::GetBackgroundDrawList(), sw, sh);
        RenderMenu();
        ImGui::Render();
        float clear[] = { 0, 0, 0, 0 };
        ctx->OMSetRenderTargets(1, &rtv, NULL);
        ctx->ClearRenderTargetView(rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        sc->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    rtv->Release(); sc->Release(); ctx->Release(); dev->Release();
    DestroyWindow(hwnd);
    g_rbx.detach();
    FreeLibraryAndExitThread((HMODULE)module, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, OverlayThread, hModule, 0, NULL);
    }
    return TRUE;
}
