#include "AnimCatalog.h"
#include "AnimPanelUI.h"
#include "VTableHook.h"

#include <windows.h>
#include <d3d9.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <algorithm>

#include <imgui.h>
#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace animpanel {

namespace {

constexpr char kCatalogPath[] = "modloader\\AnimPanel\\data\\anim-catalog.json";
constexpr char kFavoritesPath[] = "modloader\\AnimPanel\\data\\favorites.json";
constexpr char kRecentsPath[] = "modloader\\AnimPanel\\data\\recents.json";
constexpr char kFallbackCopyPath[] = "modloader\\AnimPanel\\cache\\last-selected.txt";
constexpr char kBridgePath[] = "modloader\\AnimPanel\\cache\\bridge.ini";
constexpr char kNativeLogPath[] = "modloader\\AnimPanel\\logs\\native-panel.log";
constexpr char kUiFontPath[] = "modloader\\AnimPanel\\fonts\\Rajdhani-Bold.ttf";

typedef HRESULT(__stdcall* EndSceneFn)(IDirect3DDevice9*);
typedef HRESULT(__stdcall* ResetFn)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef HRESULT(__stdcall* PresentFn)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);

AnimCatalog g_catalog;
AnimPanelState g_state;
std::unique_ptr<AnimPanelUI> g_ui;
std::unique_ptr<VTableHookManager> g_hooks;

EndSceneFn g_originalEndScene = nullptr;
ResetFn g_originalReset = nullptr;
PresentFn g_originalPresent = nullptr;

IDirect3DDevice9* g_device = nullptr;
HWND g_window = nullptr;
WNDPROC g_originalWndProc = nullptr;
bool g_imguiReady = false;
int g_bridgeSequence = 0;
bool g_lastBridgeVisible = false;
std::string g_lastBridgeSelectionId;
bool g_loggedFirstRender = false;
void** g_hookedVTable = nullptr;

bool ConsumeGlobalKeyPress(int vk) {
    static bool previous[256] = {};
    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool pressed = down && !previous[vk];
    previous[vk] = down;
    return pressed;
}

void SetStatus(const std::string& text);
void EnsureBridgeDirectories();
HRESULT __stdcall HookedReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
HRESULT __stdcall HookedPresent(IDirect3DDevice9* device, const RECT* sourceRect, const RECT* destRect, HWND overrideWindow, const RGNDATA* dirtyRegion);
HRESULT __stdcall HookedEndScene(IDirect3DDevice9* device);

void AppendNativeLog(const std::string& text) {
    EnsureBridgeDirectories();
    CreateDirectoryA("modloader\\AnimPanel\\logs", nullptr);

    std::ofstream output(kNativeLogPath, std::ios::binary | std::ios::app);
    if (!output) {
        return;
    }

    SYSTEMTIME localTime{};
    GetLocalTime(&localTime);
    char stamp[64];
    std::snprintf(
        stamp,
        sizeof(stamp),
        "%04u-%02u-%02u %02u:%02u:%02u",
        static_cast<unsigned>(localTime.wYear),
        static_cast<unsigned>(localTime.wMonth),
        static_cast<unsigned>(localTime.wDay),
        static_cast<unsigned>(localTime.wHour),
        static_cast<unsigned>(localTime.wMinute),
        static_cast<unsigned>(localTime.wSecond));
    output << "[" << stamp << "] " << text << "\n";
}

void InstallHooksForCurrentDevice(bool forceLog) {
    const auto devicePtr = *reinterpret_cast<uintptr_t*>(0xC97C28);
    if (devicePtr == 0) {
        return;
    }

    void** vTable = *reinterpret_cast<void***>(devicePtr);
    if (vTable == nullptr) {
        return;
    }

    const bool needsFreshHook =
        g_hooks == nullptr ||
        g_hookedVTable != vTable ||
        vTable[16] != reinterpret_cast<void*>(HookedReset) ||
        vTable[17] != reinterpret_cast<void*>(HookedPresent) ||
        vTable[42] != reinterpret_cast<void*>(HookedEndScene);

    if (!needsFreshHook) {
        return;
    }

    g_hooks = std::make_unique<VTableHookManager>(vTable, 119);
    g_originalReset = reinterpret_cast<ResetFn>(g_hooks->Hook(16, reinterpret_cast<void*>(HookedReset)));
    g_originalPresent = reinterpret_cast<PresentFn>(g_hooks->Hook(17, reinterpret_cast<void*>(HookedPresent)));
    g_originalEndScene = reinterpret_cast<EndSceneFn>(g_hooks->Hook(42, reinterpret_cast<void*>(HookedEndScene)));
    g_hookedVTable = vTable;

    if (forceLog) {
        AppendNativeLog("Hooks installed/refreshed for current Direct3D vtable.");
    }
}

std::string ToUpperCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

bool WriteClipboardText(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    EmptyClipboard();
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (handle == nullptr) {
        CloseClipboard();
        return false;
    }

    void* target = GlobalLock(handle);
    if (target == nullptr) {
        GlobalFree(handle);
        CloseClipboard();
        return false;
    }

    std::memcpy(target, text.c_str(), text.size() + 1);
    GlobalUnlock(handle);
    SetClipboardData(CF_TEXT, handle);
    CloseClipboard();
    return true;
}

void WriteFallbackCopy(const std::string& text) {
    std::ofstream output(kFallbackCopyPath, std::ios::binary | std::ios::trunc);
    output << text;
}

const AnimEntry* GetCurrentSelection() {
    if (g_state.filteredIndices.empty()) {
        return nullptr;
    }
    if (g_state.selectedResult < 0 || g_state.selectedResult >= static_cast<int>(g_state.filteredIndices.size())) {
        return nullptr;
    }
    return &g_catalog.Entries()[g_state.filteredIndices[static_cast<size_t>(g_state.selectedResult)]];
}

void EnsureBridgeDirectories() {
    CreateDirectoryA("modloader\\AnimPanel", nullptr);
    CreateDirectoryA("modloader\\AnimPanel\\cache", nullptr);
}

void WriteBridgeState(int commandAction, const AnimEntry* entry) {
    EnsureBridgeDirectories();

    std::ofstream output(kBridgePath, std::ios::binary | std::ios::trunc);
    if (!output) {
        SetStatus("Failed to write bridge.ini");
        AppendNativeLog("Failed to write bridge.ini");
        return;
    }

    const int visible = g_state.visible ? 1 : 0;
    output << "[panel]\n";
    output << "visible=" << visible << "\n";
    output << "category_index=" << g_state.categoryIndex << "\n";
    output << "selected_result=" << g_state.selectedResult << "\n";
    output << "\n[selection]\n";
    if (entry != nullptr) {
        const std::string ifpUpper = ToUpperCopy(entry->ifpFile);
        output << "id=" << entry->id << "\n";
        output << "display_name=" << entry->displayName << "\n";
        output << "block=" << entry->block << "\n";
        output << "anim_name=" << entry->animName << "\n";
        output << "ifp_file=" << ifpUpper << "\n";
        output << "category=" << entry->category << "\n";
        output << "loop_default=" << (entry->loopDefault ? 1 : 0) << "\n";
    } else {
        output << "id=\n";
        output << "display_name=\n";
        output << "block=\n";
        output << "anim_name=\n";
        output << "ifp_file=\n";
        output << "category=\n";
        output << "loop_default=0\n";
    }
    output << "\n[command]\n";
    output << "sequence=" << g_bridgeSequence << "\n";
    output << "action=" << commandAction << "\n";
}

void SyncBridgeSelection(bool forceWrite, int commandAction) {
    const AnimEntry* selected = GetCurrentSelection();
    const std::string selectedId = selected != nullptr ? selected->id : std::string();
    if (!forceWrite && g_lastBridgeVisible == g_state.visible && g_lastBridgeSelectionId == selectedId && commandAction == 0) {
        return;
    }

    WriteBridgeState(commandAction, selected);
    g_lastBridgeVisible = g_state.visible;
    g_lastBridgeSelectionId = selectedId;
}

void SetStatus(const std::string& text) {
    g_state.statusLine = text;
    AppendNativeLog("STATUS: " + text);
}

void SaveDirtyState() {
    if (g_state.favoritesDirty) {
        std::string error;
        g_catalog.SaveFavorites(kFavoritesPath, error);
        if (!error.empty()) {
            SetStatus(error);
        }
        g_state.favoritesDirty = false;
    }
    if (g_state.recentsDirty) {
        std::string error;
        g_catalog.SaveRecents(kRecentsPath, error);
        if (!error.empty()) {
            SetStatus(error);
        }
        g_state.recentsDirty = false;
    }
}

void EnsureUi() {
    if (g_ui != nullptr) {
        return;
    }

    std::string error;
    if (!g_catalog.LoadCatalog(kCatalogPath, error)) {
        SetStatus(error);
        AppendNativeLog("Catalog load failed: " + error);
        return;
    }

    error.clear();
    g_catalog.LoadFavorites(kFavoritesPath, error);
    error.clear();
    g_catalog.LoadRecents(kRecentsPath, error);

    SyncBridgeSelection(true, 0);

    AnimPanelCallbacks callbacks;
    callbacks.onPlay = [](const AnimEntry& entry) {
        ++g_bridgeSequence;
        WriteBridgeState(1, &entry);
        g_lastBridgeVisible = g_state.visible;
        g_lastBridgeSelectionId = entry.id;
        SetStatus("Previewing " + entry.block + "/" + entry.animName);
    };
    callbacks.onStop = []() {
        ++g_bridgeSequence;
        WriteBridgeState(2, GetCurrentSelection());
        g_lastBridgeVisible = g_state.visible;
        const AnimEntry* selected = GetCurrentSelection();
        g_lastBridgeSelectionId = selected != nullptr ? selected->id : std::string();
        SetStatus("Preview stopped.");
    };
    callbacks.onCopy = [](const AnimEntry& entry) {
        const std::string payload = entry.block + "/" + entry.animName + " | " + entry.ifpFile + ":" + entry.animName;
        if (!WriteClipboardText(payload)) {
            WriteFallbackCopy(payload);
            SetStatus("Clipboard unavailable. Wrote fallback copy file.");
            return;
        }
        WriteFallbackCopy(payload);
        SetStatus("Animation metadata copied.");
    };

    g_ui = std::make_unique<AnimPanelUI>(g_catalog, g_state, std::move(callbacks));
    AppendNativeLog("UI created and catalog loaded.");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_imguiReady) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
            return 1;
        }

        if (g_state.visible && g_state.wantsInputBlock) {
            switch (msg) {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_CHAR:
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                return 1;
            default:
                break;
            }
        }
    }

    return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}

void InitializeImGui(IDirect3DDevice9* device) {
    if (g_imguiReady) {
        return;
    }

    D3DDEVICE_CREATION_PARAMETERS params{};
    if (FAILED(device->GetCreationParameters(&params))) {
        return;
    }

    g_window = params.hFocusWindow;
    g_device = device;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->Clear();
    if (io.Fonts->AddFontFromFileTTF(kUiFontPath, 24.0f) == nullptr) {
        io.Fonts->AddFontDefault();
        AppendNativeLog("Custom font load failed, using default font.");
    } else {
        AppendNativeLog("Loaded custom UI font.");
    }

    ImGui_ImplWin32_Init(g_window);
    ImGui_ImplDX9_Init(device);
    g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
    g_imguiReady = true;
    AppendNativeLog("ImGui initialized.");
}

void ShutdownImGui() {
    if (!g_imguiReady) {
        return;
    }

    SaveDirtyState();

    if (g_originalWndProc != nullptr && g_window != nullptr) {
        SetWindowLongPtr(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_originalWndProc));
        g_originalWndProc = nullptr;
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_imguiReady = false;
    g_window = nullptr;
    g_device = nullptr;
}

HRESULT __stdcall HookedReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) {
    if (g_imguiReady) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }
    const HRESULT result = g_originalReset(device, params);
    if (SUCCEEDED(result) && g_imguiReady) {
        ImGui_ImplDX9_CreateDeviceObjects();
    }
    return result;
}

void RenderFrame(IDirect3DDevice9* device) {
    InitializeImGui(device);
    EnsureUi();

    if (!g_loggedFirstRender) {
        AppendNativeLog("First render callback reached.");
        g_loggedFirstRender = true;
    }

    if (ConsumeGlobalKeyPress(VK_F8)) {
        g_state.visible = !g_state.visible;
        SyncBridgeSelection(true, 0);
    }

    if (g_state.visible && ConsumeGlobalKeyPress(VK_ESCAPE)) {
        g_state.visible = false;
        SyncBridgeSelection(true, 0);
    }

    if (g_state.visible && g_window != nullptr && GetForegroundWindow() != g_window) {
        g_state.visible = false;
        SyncBridgeSelection(true, 0);
    }

    if (g_imguiReady && g_ui != nullptr) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        g_ui->Render();
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        SaveDirtyState();
        SyncBridgeSelection(false, 0);
    }
}

HRESULT __stdcall HookedEndScene(IDirect3DDevice9* device) {
    RenderFrame(device);
    return g_originalEndScene(device);
}

HRESULT __stdcall HookedPresent(IDirect3DDevice9* device, const RECT* sourceRect, const RECT* destRect, HWND overrideWindow, const RGNDATA* dirtyRegion) {
    RenderFrame(device);
    return g_originalPresent(device, sourceRect, destRect, overrideWindow, dirtyRegion);
}

DWORD WINAPI MainThread(LPVOID) {
    AppendNativeLog("AnimPanel thread started.");
    while (GetModuleHandleA("d3d9.dll") == nullptr) {
        Sleep(50);
    }
    AppendNativeLog("d3d9.dll detected.");

    bool loggedDeviceReady = false;
    for (;;) {
        const auto devicePtr = *reinterpret_cast<uintptr_t*>(0xC97C28);
        if (devicePtr != 0 && !loggedDeviceReady) {
            AppendNativeLog("Direct3D device pointer acquired.");
            loggedDeviceReady = true;
        }

        InstallHooksForCurrentDevice(loggedDeviceReady);
        Sleep(250);
    }

    return 0;
}

} // namespace

} // namespace animpanel

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    UNREFERENCED_PARAMETER(reserved);

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        DeleteFileA("modloader\\AnimPanel\\logs\\native-panel.log");
        CreateThread(nullptr, 0, animpanel::MainThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        animpanel::ShutdownImGui();
        break;
    default:
        break;
    }

    return TRUE;
}
