#include "AnimCatalog.h"
#include "AnimPanelUI.h"
#include "VTableHook.h"

#include <windows.h>
#include <d3d9.h>
#include <plugin.h>
#include <RenderWare.h>
#include <CWorld.h>
#include <CPools.h>
#include <extensions/ScriptCommands.h>
#include <eScriptCommands.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <algorithm>
#include <cmath>
#include <unordered_set>

#include <imgui.h>
#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../third_party/plugin-sdk/stb/stb_image.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace plugin;

namespace animpanel {

namespace {

constexpr char kRuntimeRoot[] = "AnimPanel";
constexpr char kLegacyRuntimeRoot[] = "modloader\\AnimPanel";
constexpr char kCatalogRelativePath[] = "data\\anim-catalog.json";
constexpr char kFavoritesRelativePath[] = "data\\favorites.json";
constexpr char kRecentsRelativePath[] = "data\\recents.json";
constexpr char kSettingsRelativePath[] = "data\\settings.ini";
constexpr char kFallbackCopyRelativePath[] = "cache\\last-selected.txt";
constexpr char kNativeLogRelativePath[] = "logs\\native-panel.log";
constexpr char kFaultAnimsRelativePath[] = "logs\\faultanims.txt";
constexpr char kUiFontRelativePath[] = "fonts\\Rajdhani-Bold.ttf";
constexpr char kFavoriteStarRelativePath[] = "images\\star.png";
constexpr float kPi = 3.1415926535f;
constexpr DWORD kVerifyDelayMs = 100;
constexpr float kFacingHeading = 180.0f;
constexpr float kCameraDistance = 3.55f;
constexpr float kCameraHeight = 0.82f;
constexpr float kCameraTargetHeight = 0.60f;

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
bool g_loggedFirstRender = false;
void** g_hookedVTable = nullptr;
bool g_nativeTickRegistered = false;
bool g_cameraApplied = false;
bool g_panelControlLocked = false;
bool g_previewActive = false;
bool g_previousVisible = false;
std::string g_loadedIfp;
AnimEntry g_pendingEntry;
AnimEntry g_activeEntry;
bool g_hasPendingEntry = false;
bool g_hasActiveEntry = false;
std::unordered_set<std::string> g_faultedAnimIds;
std::string g_attemptIfps[4];
bool g_attemptUseTaskPlayAnim[4] = {};
int g_attemptCount = 0;
int g_attemptIndex = -1;
DWORD g_attemptStartedAt = 0;
bool g_panelJustOpened = false;
LPDIRECT3DTEXTURE9 g_favoriteStarTexture = nullptr;

bool ConsumeGlobalKeyPress(int vk) {
    static bool previous[256] = {};
    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool pressed = down && !previous[vk];
    previous[vk] = down;
    return pressed;
}

void SetStatus(const std::string& text);
void EnsureRuntimeDirectories();
HRESULT __stdcall HookedReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
HRESULT __stdcall HookedPresent(IDirect3DDevice9* device, const RECT* sourceRect, const RECT* destRect, HWND overrideWindow, const RGNDATA* dirtyRegion);
HRESULT __stdcall HookedEndScene(IDirect3DDevice9* device);
void ProcessGameplayFrame();
void ApplyPanelControlLock();
void ReleasePanelControlLock();
void QueuePreviewEntry(const AnimEntry& entry, const char* reason);
void ReleaseFavoriteStarTexture();
void TryLoadFavoriteStarTexture(IDirect3DDevice9* device);

bool PathExists(const std::string& path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool DirectoryExists(const std::string& path) {
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::string JoinPath(const std::string& root, const char* relative) {
    if (root.empty()) {
        return relative;
    }
    return root + "\\" + relative;
}

std::string ResolveReadPath(const char* relative) {
    const std::string preferred = JoinPath(kRuntimeRoot, relative);
    if (PathExists(preferred)) {
        return preferred;
    }

    return JoinPath(kLegacyRuntimeRoot, relative);
}

std::string ResolveWritePath(const char* relative) {
    if (DirectoryExists(kRuntimeRoot) || !DirectoryExists(kLegacyRuntimeRoot)) {
        return JoinPath(kRuntimeRoot, relative);
    }

    return JoinPath(kLegacyRuntimeRoot, relative);
}

void EnsureDirectoryForWrite(const std::string& relativeDir) {
    if (DirectoryExists(kRuntimeRoot) || !DirectoryExists(kLegacyRuntimeRoot)) {
        CreateDirectoryA(kRuntimeRoot, nullptr);
        CreateDirectoryA(JoinPath(kRuntimeRoot, relativeDir.c_str()).c_str(), nullptr);
        return;
    }

    CreateDirectoryA(kLegacyRuntimeRoot, nullptr);
    CreateDirectoryA(JoinPath(kLegacyRuntimeRoot, relativeDir.c_str()).c_str(), nullptr);
}

void AppendNativeLog(const std::string& text) {
    EnsureRuntimeDirectories();
    const std::string logPath = ResolveWritePath(kNativeLogRelativePath);
    std::ofstream output(logPath, std::ios::binary | std::ios::app);
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

void ReleaseFavoriteStarTexture() {
    if (g_favoriteStarTexture != nullptr) {
        g_favoriteStarTexture->Release();
        g_favoriteStarTexture = nullptr;
    }
    g_state.favoriteIconTexture = nullptr;
    g_state.favoriteIconWidth = 0.0f;
    g_state.favoriteIconHeight = 0.0f;
}

void TryLoadFavoriteStarTexture(IDirect3DDevice9* device) {
    if (device == nullptr || g_favoriteStarTexture != nullptr) {
        return;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    const std::string iconPath = ResolveReadPath(kFavoriteStarRelativePath);
    stbi_uc* rgba = stbi_load(iconPath.c_str(), &width, &height, &channels, 4);
    if (rgba == nullptr) {
        AppendNativeLog("Favorite star texture load failed: " + iconPath);
        return;
    }

    if (FAILED(device->CreateTexture(
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &g_favoriteStarTexture,
        nullptr))) {
        stbi_image_free(rgba);
        AppendNativeLog("Favorite star Direct3D texture creation failed.");
        return;
    }

    D3DLOCKED_RECT locked{};
    if (FAILED(g_favoriteStarTexture->LockRect(0, &locked, nullptr, 0))) {
        g_favoriteStarTexture->Release();
        g_favoriteStarTexture = nullptr;
        stbi_image_free(rgba);
        AppendNativeLog("Favorite star texture lock failed.");
        return;
    }

    for (int y = 0; y < height; ++y) {
        const stbi_uc* src = rgba + (y * width * 4);
        auto* dst = reinterpret_cast<std::uint32_t*>(static_cast<unsigned char*>(locked.pBits) + (y * locked.Pitch));
        for (int x = 0; x < width; ++x) {
            const std::uint8_t r = src[x * 4 + 0];
            const std::uint8_t g = src[x * 4 + 1];
            const std::uint8_t b = src[x * 4 + 2];
            const std::uint8_t a = src[x * 4 + 3];
            dst[x] =
                (static_cast<std::uint32_t>(a) << 24) |
                (static_cast<std::uint32_t>(r) << 16) |
                (static_cast<std::uint32_t>(g) << 8) |
                static_cast<std::uint32_t>(b);
        }
    }

    g_favoriteStarTexture->UnlockRect(0);
    stbi_image_free(rgba);
    g_state.favoriteIconTexture = g_favoriteStarTexture;
    g_state.favoriteIconWidth = static_cast<float>(width);
    g_state.favoriteIconHeight = static_cast<float>(height);
    AppendNativeLog("Loaded favorite star texture: " + std::to_string(width) + "x" + std::to_string(height));
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
    std::ofstream output(ResolveWritePath(kFallbackCopyRelativePath), std::ios::binary | std::ios::trunc);
    output << text;
}

std::string MakeFaultKey(const AnimEntry& entry) {
    return entry.id.empty() ? (entry.ifpFile + ":" + entry.animName) : entry.id;
}

void AppendFaultAnimLine(const AnimEntry& entry, const std::string& reason) {
    const std::string key = MakeFaultKey(entry);
    if (g_faultedAnimIds.find(key) != g_faultedAnimIds.end()) {
        return;
    }

    g_faultedAnimIds.insert(key);
    EnsureRuntimeDirectories();
    std::ofstream output(ResolveWritePath(kFaultAnimsRelativePath), std::ios::binary | std::ios::app);
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

    output << "[" << stamp << "] "
           << entry.displayName << " | "
           << entry.ifpFile << "/" << entry.animName << " | "
           << reason << "\n";
}

void LoadSettings() {
    g_state.autoPlayEnabled = false;
    g_state.fastModeEnabled = false;

    std::ifstream input(ResolveReadPath(kSettingsRelativePath), std::ios::binary);
    if (!input) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("auto_play=", 0) == 0) {
            g_state.autoPlayEnabled = line.substr(10) == "1";
        } else if (line.rfind("fast_mode=", 0) == 0) {
            g_state.fastModeEnabled = line.substr(10) == "1";
        }
    }
}

void SaveSettings() {
    EnsureRuntimeDirectories();
    std::ofstream output(ResolveWritePath(kSettingsRelativePath), std::ios::binary | std::ios::trunc);
    if (!output) {
        return;
    }

    output << "auto_play=" << (g_state.autoPlayEnabled ? "1" : "0") << "\n";
    output << "fast_mode=" << (g_state.fastModeEnabled ? "1" : "0") << "\n";
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

void EnsureRuntimeDirectories() {
    EnsureDirectoryForWrite("cache");
    EnsureDirectoryForWrite("logs");
    EnsureDirectoryForWrite("data");
}

void SetStatus(const std::string& text) {
    g_state.statusLine = text;
    AppendNativeLog("STATUS: " + text);
}

bool IsKnownFaultAnimation(const AnimEntry& entry) {
    return g_faultedAnimIds.find(MakeFaultKey(entry)) != g_faultedAnimIds.end();
}

bool IsPlayerPreviewBlocked(int playerHandle) {
    if (Command<Commands::IS_CHAR_IN_ANY_CAR>(playerHandle) ||
        Command<Commands::IS_CHAR_IN_WATER>(playerHandle) ||
        Command<Commands::IS_PLAYER_USING_JETPACK>(0)) {
        return true;
    }
    return false;
}

void RestorePreviewCamera() {
    if (!g_cameraApplied) {
        return;
    }
    Command<Commands::RESTORE_CAMERA_JUMPCUT>();
    g_cameraApplied = false;
}

void UpdatePreviewCamera() {
    if (!g_state.visible) {
        RestorePreviewCamera();
        return;
    }

    auto* player = FindPlayerPed();
    if (!player) {
        return;
    }

    const int hPlayer = CPools::GetPedRef(player);
    if (IsPlayerPreviewBlocked(hPlayer)) {
        RestorePreviewCamera();
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    Command<Commands::GET_CHAR_COORDINATES>(hPlayer, &x, &y, &z);
    const float heading = kFacingHeading;

    const float radians = heading * (kPi / 180.0f);
    const float cameraX = x + std::sin(radians) * kCameraDistance;
    const float cameraY = y + std::cos(radians) * kCameraDistance;
    const float cameraZ = z + kCameraHeight;

    Command<Commands::SET_FIXED_CAMERA_POSITION>(cameraX, cameraY, cameraZ, 0.0f, 0.0f);
    Command<Commands::POINT_CAMERA_AT_POINT>(x, y, z + kCameraTargetHeight, 2);
    g_cameraApplied = true;
}

void ApplyPanelControlLock() {
    if (g_panelControlLocked) {
        return;
    }
    Command<Commands::SET_PLAYER_CONTROL>(0, 0);
    Command<Commands::SET_PLAYER_ENTER_CAR_BUTTON>(0, 0);
    g_panelControlLocked = true;
}

void ReleasePanelControlLock() {
    if (!g_panelControlLocked) {
        return;
    }
    Command<Commands::SET_PLAYER_CONTROL>(0, 1);
    Command<Commands::SET_PLAYER_ENTER_CAR_BUTTON>(0, 1);
    g_panelControlLocked = false;
}

void ReleaseLoadedIfp() {
    if (!g_loadedIfp.empty() && g_loadedIfp != "PED") {
        Command<Commands::REMOVE_ANIMATION>(g_loadedIfp.c_str());
    }
    g_loadedIfp.clear();
}

void StopPreviewInternal(bool restoreCamera, const char* reason) {
    auto* player = FindPlayerPed();
    if (player) {
        const int hPlayer = CPools::GetPedRef(player);
        Command<Commands::CLEAR_CHAR_TASKS_IMMEDIATELY>(hPlayer);
    }
    Command<Commands::SET_PLAYER_ENTER_CAR_BUTTON>(0, g_state.visible ? 0 : 1);
    ReleaseLoadedIfp();
    g_previewActive = false;
    g_hasPendingEntry = false;
    g_hasActiveEntry = false;
    g_attemptCount = 0;
    g_attemptIndex = -1;
    g_attemptStartedAt = 0;
    if (restoreCamera) {
        RestorePreviewCamera();
    }
    if (reason && reason[0] != '\0') {
        AppendNativeLog(std::string("Preview stopped: ") + reason);
    }
}

void MoveSelectionToNextAnimation(bool queuePreview) {
    if (g_state.filteredIndices.empty()) {
        return;
    }

    bool moved = false;
    if (g_state.selectedResult + 1 < static_cast<int>(g_state.filteredIndices.size())) {
        ++g_state.selectedResult;
        moved = true;
    }

    if (!queuePreview || !moved) {
        return;
    }

    const AnimEntry* next = GetCurrentSelection();
    if (next != nullptr) {
        QueuePreviewEntry(*next, "fault skip");
        g_catalog.MarkRecent(g_state.filteredIndices[static_cast<size_t>(g_state.selectedResult)]);
        g_state.recentsDirty = true;
    }
}

void HandleAnimationFault(const AnimEntry& entry, const std::string& reason) {
    AppendFaultAnimLine(entry, reason);
    AppendNativeLog("Faulted animation: " + entry.ifpFile + "/" + entry.animName + " reason=" + reason);
    SetStatus("Animation failed. Skipping.");
    StopPreviewInternal(false, reason.c_str());
    MoveSelectionToNextAnimation(g_state.autoPlayEnabled && g_state.visible && g_state.viewMode == 1);
}

void BuildPlayAttempts(const AnimEntry& entry) {
    g_attemptCount = 0;
    g_attemptIndex = -1;

    const std::string requestIfp = ToUpperCopy(entry.ifpFile);
    const std::string primaryIfp = entry.pedFlag ? (entry.block.empty() ? "PED" : entry.block) : (entry.block.empty() ? requestIfp : entry.block);
    const std::string fallbackIfp = requestIfp;

    auto pushAttempt = [](const std::string& ifp, bool useTaskPlayAnim) {
        for (int i = 0; i < g_attemptCount; ++i) {
            if (g_attemptIfps[i] == ifp && g_attemptUseTaskPlayAnim[i] == useTaskPlayAnim) {
                return;
            }
        }
        g_attemptIfps[g_attemptCount] = ifp;
        g_attemptUseTaskPlayAnim[g_attemptCount] = useTaskPlayAnim;
        ++g_attemptCount;
    };

    pushAttempt(primaryIfp, false);
    pushAttempt(primaryIfp, true);
    pushAttempt(fallbackIfp, false);
    pushAttempt(fallbackIfp, true);
}

bool PrepareAnimationResources(const AnimEntry& entry) {
    const std::string requestIfp = ToUpperCopy(entry.ifpFile);
    if (entry.pedFlag || requestIfp == "PED") {
        ReleaseLoadedIfp();
        return true;
    }

    if (g_loadedIfp != requestIfp) {
        ReleaseLoadedIfp();
        Command<Commands::REQUEST_ANIMATION>(requestIfp.c_str());
        Command<Commands::LOAD_ALL_MODELS_NOW>();
        if (!Command<Commands::HAS_ANIMATION_LOADED>(requestIfp.c_str())) {
            AppendNativeLog("Failed to load animation IFP: " + requestIfp);
            SetStatus("Failed to load animation block.");
            return false;
        }
        g_loadedIfp = requestIfp;
    }
    return true;
}

bool TryIssuePlayOpcode(int hPlayer, const char* anim, const char* ifp, bool loopDefault, bool lockF, bool useTaskPlayAnim) {
    __try {
        Command<Commands::SET_PLAYER_ENTER_CAR_BUTTON>(0, 0);
        Command<Commands::CLEAR_CHAR_TASKS_IMMEDIATELY>(hPlayer);

        if (useTaskPlayAnim) {
            Command<Commands::TASK_PLAY_ANIM>(hPlayer, anim, ifp, 4.0f, loopDefault, 0, 0, lockF, -1);
        } else {
            Command<Commands::TASK_PLAY_ANIM_NON_INTERRUPTABLE>(hPlayer, anim, ifp, 4.0f, loopDefault, 0, 0, lockF, -1);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void IssuePlayAttempt(CPlayerPed* player) {
    if (!player || g_attemptIndex < 0 || g_attemptIndex >= g_attemptCount) {
        return;
    }

    const int hPlayer = CPools::GetPedRef(player);
    const std::string& anim = g_pendingEntry.animName;
    const std::string& ifp = g_attemptIfps[g_attemptIndex];
    const bool useTaskPlayAnim = g_attemptUseTaskPlayAnim[g_attemptIndex];

    AppendNativeLog(std::string("Play attempt #") + std::to_string(g_attemptIndex + 1) +
        " anim=" + anim + " ifp=" + ifp + (useTaskPlayAnim ? " opcode=0605" : " opcode=0812"));

    if (!TryIssuePlayOpcode(hPlayer, anim.c_str(), ifp.c_str(), g_pendingEntry.loopDefault, g_pendingEntry.lockF, useTaskPlayAnim)) {
        HandleAnimationFault(g_pendingEntry, "Immediate native exception during play call");
        return;
    }
    g_attemptStartedAt = GetTickCount();
}

void AdvancePendingPreview() {
    if (!g_hasPendingEntry) {
        return;
    }

    auto* player = FindPlayerPed();
    if (!player) {
        return;
    }

    const int hPlayer = CPools::GetPedRef(player);
    if (IsPlayerPreviewBlocked(hPlayer)) {
        SetStatus("Preview requires CJ on foot and out of water.");
        StopPreviewInternal(false, "player state blocked preview");
        return;
    }

    if (g_attemptIndex < 0) {
        if (!PrepareAnimationResources(g_pendingEntry)) {
            StopPreviewInternal(false, "animation resources failed");
            return;
        }
        BuildPlayAttempts(g_pendingEntry);
        g_attemptIndex = 0;
        IssuePlayAttempt(player);
        return;
    }

    if (GetTickCount() - g_attemptStartedAt < kVerifyDelayMs) {
        return;
    }

    if (Command<Commands::IS_CHAR_PLAYING_ANIM>(hPlayer, g_pendingEntry.animName.c_str())) {
        g_previewActive = true;
        g_hasActiveEntry = true;
        g_activeEntry = g_pendingEntry;
        g_hasPendingEntry = false;
        AppendNativeLog("Preview active: " + g_activeEntry.ifpFile + "/" + g_activeEntry.animName);
        return;
    }

    ++g_attemptIndex;
    if (g_attemptIndex >= g_attemptCount) {
        HandleAnimationFault(g_pendingEntry, "All native playback attempts failed");
        return;
    }

    IssuePlayAttempt(player);
}

void ProcessGameplayFrame() {
    if (!g_previousVisible && g_state.visible) {
        g_panelJustOpened = true;
        ApplyPanelControlLock();
    }

    if (g_previousVisible && !g_state.visible) {
        StopPreviewInternal(true, "panel closed");
        ReleasePanelControlLock();
        g_panelJustOpened = false;
    }
    g_previousVisible = g_state.visible;

    if (!g_nativeTickRegistered) {
        return;
    }

    if (g_state.visible) {
        ApplyPanelControlLock();
        if (g_panelJustOpened) {
            auto* player = FindPlayerPed();
            if (player) {
                const int hPlayer = CPools::GetPedRef(player);
                if (!IsPlayerPreviewBlocked(hPlayer)) {
                    Command<Commands::SET_CHAR_HEADING>(hPlayer, kFacingHeading);
                }
            }
            g_panelJustOpened = false;
        }
        UpdatePreviewCamera();
    } else {
        ReleasePanelControlLock();
        RestorePreviewCamera();
    }

    AdvancePendingPreview();
}

void SaveDirtyState() {
    if (g_state.favoritesDirty) {
        std::string error;
        g_catalog.SaveFavorites(ResolveWritePath(kFavoritesRelativePath), error);
        if (!error.empty()) {
            SetStatus(error);
        }
        g_state.favoritesDirty = false;
    }
    if (g_state.recentsDirty) {
        std::string error;
        g_catalog.SaveRecents(ResolveWritePath(kRecentsRelativePath), error);
        if (!error.empty()) {
            SetStatus(error);
        }
        g_state.recentsDirty = false;
    }
    if (g_state.settingsDirty) {
        SaveSettings();
        g_state.settingsDirty = false;
    }
}

void EnsureUi() {
    if (g_ui != nullptr) {
        return;
    }

    std::string error;
    if (!g_catalog.LoadCatalog(ResolveReadPath(kCatalogRelativePath), error)) {
        SetStatus(error);
        AppendNativeLog("Catalog load failed: " + error);
        return;
    }

    error.clear();
    g_catalog.LoadFavorites(ResolveWritePath(kFavoritesRelativePath), error);
    error.clear();
    g_catalog.LoadRecents(ResolveWritePath(kRecentsRelativePath), error);
    LoadSettings();

    AnimPanelCallbacks callbacks;
    callbacks.onPlay = [](const AnimEntry& entry) {
        QueuePreviewEntry(entry, "new selection");
    };
    callbacks.onStop = []() {
        StopPreviewInternal(true, "manual stop");
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

void QueuePreviewEntry(const AnimEntry& entry, const char* reason) {
    if (IsKnownFaultAnimation(entry)) {
        SetStatus("Known fault animation skipped.");
        AppendNativeLog("Skipped known fault animation: " + entry.ifpFile + "/" + entry.animName);
        if (g_state.autoPlayEnabled && g_state.visible && g_state.viewMode == 1) {
            MoveSelectionToNextAnimation(true);
        }
        return;
    }

    StopPreviewInternal(false, reason);
    g_pendingEntry = entry;
    g_hasPendingEntry = true;
    SetStatus("Previewing " + entry.block + "/" + entry.animName);
    AppendNativeLog("Queued native preview: " + entry.ifpFile + "/" + entry.animName + " block=" + entry.block);
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
    const std::string fontPath = ResolveReadPath(kUiFontRelativePath);
    if (io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 24.0f) == nullptr) {
        io.Fonts->AddFontDefault();
        AppendNativeLog("Custom font load failed, using default font.");
    } else {
        AppendNativeLog("Loaded custom UI font.");
    }

    ImGui_ImplWin32_Init(g_window);
    ImGui_ImplDX9_Init(device);
    TryLoadFavoriteStarTexture(device);
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

    ReleaseFavoriteStarTexture();
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
    }

    if (g_state.visible && ConsumeGlobalKeyPress(VK_ESCAPE)) {
        g_state.visible = false;
    }

    if (g_state.visible && g_window != nullptr && GetForegroundWindow() != g_window) {
        g_state.visible = false;
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
    if (!g_nativeTickRegistered) {
        plugin::Events::processScriptsEvent += [] {
            ProcessGameplayFrame();
        };
        g_nativeTickRegistered = true;
        AppendNativeLog("Native gameplay tick registered.");
    }

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
        DeleteFileA("AnimPanel\\logs\\native-panel.log");
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
