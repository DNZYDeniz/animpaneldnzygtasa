#include "AnimPanelUI.h"

#include <windows.h>
#include <imgui.h>

#include <string>
#include <utility>

namespace animpanel {

namespace {

constexpr int kViewCategories = 0;
constexpr int kViewAnimations = 1;
constexpr int kViewSettings = 2;
constexpr int kVisibleRows = 12;
constexpr DWORD kFastModeInitialDelayMs = 220;
constexpr DWORD kFastModeRepeatDelayMs = 55;

bool ConsumeKeyPress(int vk) {
    static bool previous[256] = {};
    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool pressed = down && !previous[vk];
    previous[vk] = down;
    return pressed;
}

bool ConsumeAnyKeyPress(std::initializer_list<int> keys) {
    bool pressed = false;
    for (const int key : keys) {
        if (ConsumeKeyPress(key)) {
            pressed = true;
        }
    }
    return pressed;
}

bool IsAnyKeyDown(std::initializer_list<int> keys) {
    for (const int key : keys) {
        if ((GetAsyncKeyState(key) & 0x8000) != 0) {
            return true;
        }
    }
    return false;
}

bool ConsumeRepeatKeyPress(const char* slotId, std::initializer_list<int> keys, bool fastModeEnabled) {
    struct RepeatState {
        std::string id;
        bool held = false;
        DWORD nextTick = 0;
    };
    static std::vector<RepeatState> states;

    RepeatState* state = nullptr;
    for (auto& item : states) {
        if (item.id == slotId) {
            state = &item;
            break;
        }
    }
    if (state == nullptr) {
        states.push_back(RepeatState{ slotId, false, 0 });
        state = &states.back();
    }

    const bool down = IsAnyKeyDown(keys);
    if (!down) {
        state->held = false;
        state->nextTick = 0;
        return false;
    }

    const DWORD now = GetTickCount();
    if (!state->held) {
        state->held = true;
        state->nextTick = now + (fastModeEnabled ? kFastModeInitialDelayMs : 0x7fffffff);
        return true;
    }

    if (!fastModeEnabled) {
        return false;
    }

    if (now >= state->nextTick) {
        state->nextTick = now + kFastModeRepeatDelayMs;
        return true;
    }

    return false;
}

void DrawBoldText(ImDrawList* drawList, const ImVec2& pos, ImU32 color, const char* text, float offset = 0.8f) {
    drawList->AddText(pos, color, text);
    drawList->AddText(ImVec2(pos.x + offset, pos.y), color, text);
}

} // namespace

AnimPanelUI::AnimPanelUI(AnimCatalog& catalog, AnimPanelState& state, AnimPanelCallbacks callbacks)
    : m_catalog(catalog), m_state(state), m_callbacks(std::move(callbacks)) {
    m_categories = m_catalog.MenuCategories();
    m_categories.push_back("Settings");
    RefreshResults();
}

void AnimPanelUI::RefreshResults() {
    if (m_categories.empty()) {
        m_state.filteredIndices.clear();
        m_state.selectedResult = 0;
        return;
    }

    if (m_state.categoryIndex < 0) {
        m_state.categoryIndex = 0;
    }
    if (m_state.categoryIndex >= static_cast<int>(m_categories.size())) {
        m_state.categoryIndex = static_cast<int>(m_categories.size() - 1);
    }
    if (m_state.categoryCursor < 0) {
        m_state.categoryCursor = 0;
    }
    if (m_state.categoryCursor >= static_cast<int>(m_categories.size())) {
        m_state.categoryCursor = m_state.categoryIndex;
    }

    if (IsSettingsCategory(m_state.categoryIndex)) {
        m_state.filteredIndices.clear();
        m_state.selectedResult = 0;
        return;
    }

    m_state.filteredIndices = m_catalog.Query("", GetCategoryLabel(m_state.categoryIndex));
    if (m_state.filteredIndices.empty()) {
        m_state.selectedResult = 0;
        return;
    }
    if (m_state.selectedResult < 0) {
        m_state.selectedResult = 0;
    }
    if (m_state.selectedResult >= static_cast<int>(m_state.filteredIndices.size())) {
        m_state.selectedResult = static_cast<int>(m_state.filteredIndices.size() - 1);
    }
}

void AnimPanelUI::HandleKeyboard() {
    if (!m_state.visible) {
        return;
    }

    bool selectionChanged = false;

    if (ConsumeRepeatKeyPress("nav_up", { VK_NUMPAD8, VK_UP }, m_state.fastModeEnabled)) {
        if (m_state.viewMode == kViewCategories) {
            m_state.categoryCursor -= 1;
            if (m_state.categoryCursor < 0) {
                m_state.categoryCursor = static_cast<int>(m_categories.size() - 1);
            }
        } else if (m_state.viewMode == kViewSettings) {
            m_state.settingsCursor -= 1;
            if (m_state.settingsCursor < 0) {
                m_state.settingsCursor = 1;
            }
        } else if (!m_state.filteredIndices.empty()) {
            m_state.selectedResult -= 1;
            if (m_state.selectedResult < 0) {
                m_state.selectedResult = static_cast<int>(m_state.filteredIndices.size() - 1);
            }
            selectionChanged = true;
        }
    }

    if (ConsumeRepeatKeyPress("nav_down", { VK_NUMPAD2, VK_DOWN }, m_state.fastModeEnabled)) {
        if (m_state.viewMode == kViewCategories) {
            m_state.categoryCursor += 1;
            if (m_state.categoryCursor >= static_cast<int>(m_categories.size())) {
                m_state.categoryCursor = 0;
            }
        } else if (m_state.viewMode == kViewSettings) {
            m_state.settingsCursor += 1;
            if (m_state.settingsCursor > 1) {
                m_state.settingsCursor = 0;
            }
        } else if (!m_state.filteredIndices.empty()) {
            m_state.selectedResult += 1;
            if (m_state.selectedResult >= static_cast<int>(m_state.filteredIndices.size())) {
                m_state.selectedResult = 0;
            }
            selectionChanged = true;
        }
    }

    if (ConsumeAnyKeyPress({ VK_NUMPAD4, VK_LEFT }) && m_state.viewMode == kViewAnimations && !m_state.filteredIndices.empty()) {
        m_state.selectedResult -= kVisibleRows;
        if (m_state.selectedResult < 0) {
            m_state.selectedResult = 0;
        }
        selectionChanged = true;
    }

    if (ConsumeAnyKeyPress({ VK_NUMPAD6, VK_RIGHT }) && m_state.viewMode == kViewAnimations && !m_state.filteredIndices.empty()) {
        m_state.selectedResult += kVisibleRows;
        if (m_state.selectedResult >= static_cast<int>(m_state.filteredIndices.size())) {
            m_state.selectedResult = static_cast<int>(m_state.filteredIndices.size() - 1);
        }
        selectionChanged = true;
    }

    if (ConsumeAnyKeyPress({ VK_NUMPAD5, VK_CLEAR, VK_RETURN })) {
        if (m_state.viewMode == kViewCategories) {
            if (IsSettingsCategory(m_state.categoryCursor)) {
                m_state.viewMode = kViewSettings;
                m_state.settingsCursor = 0;
                m_state.statusLine = "Opened Settings.";
            } else {
                m_state.categoryIndex = m_state.categoryCursor;
                m_state.selectedResult = 0;
                RefreshResults();
                m_state.viewMode = kViewAnimations;
                m_state.statusLine = std::string("Opened ") + GetCategoryLabel(m_state.categoryIndex);
                selectionChanged = m_state.autoPlayEnabled && !m_state.filteredIndices.empty();
            }
        } else if (m_state.viewMode == kViewSettings) {
            if (m_state.settingsCursor == 0) {
                m_state.autoPlayEnabled = !m_state.autoPlayEnabled;
                m_state.statusLine = std::string("Auto Play ") + (m_state.autoPlayEnabled ? "enabled." : "disabled.");
            } else {
                m_state.fastModeEnabled = !m_state.fastModeEnabled;
                m_state.statusLine = std::string("Fast Mode ") + (m_state.fastModeEnabled ? "enabled." : "disabled.");
            }
            m_state.settingsDirty = true;
        } else {
            TriggerPreviewForSelection();
        }
    }

    if (ConsumeKeyPress(VK_BACK)) {
        if (m_state.viewMode == kViewAnimations) {
            m_state.viewMode = kViewCategories;
            m_state.statusLine = "Returned to categories.";
        } else if (m_state.viewMode == kViewSettings) {
            m_state.viewMode = kViewCategories;
            m_state.statusLine = "Returned to categories.";
        } else if (m_callbacks.onStop) {
            m_callbacks.onStop();
        }
    }

    if (selectionChanged && m_state.viewMode == kViewAnimations && m_state.autoPlayEnabled && !m_state.filteredIndices.empty()) {
        TriggerPreviewForSelection();
    }
}

void AnimPanelUI::Render() {
    if (!m_state.visible) {
        m_state.wantsInputBlock = false;
        return;
    }

    HandleKeyboard();
    ApplyStyle();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 workPos = viewport->WorkPos;
    const ImVec2 workSize = viewport->WorkSize;
    const float panelHeight = workSize.y * 0.82f;
    const ImVec2 panelSize(378.0f, panelHeight);

    ImGui::SetNextWindowPos(ImVec2(workPos.x + 26.0f, workPos.y + (workSize.y - panelHeight) * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;

    if (!ImGui::Begin("AnimPanel##Root", &m_state.visible, flags)) {
        ImGui::End();
        return;
    }

    m_state.wantsInputBlock = true;
    ImGui::SetWindowFontScale(1.34f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float footerHeight = 106.0f;
    const float headerHeight = 72.0f;

    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(14, 19, 28, 236), 18.0f);
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + headerHeight), IM_COL32(218, 144, 34, 248), 18.0f, ImDrawFlags_RoundCornersTop);
    drawList->AddRectFilled(ImVec2(origin.x, origin.y + headerHeight), ImVec2(origin.x + size.x, origin.y + headerHeight + 24.0f), IM_COL32(218, 223, 229, 228));
    drawList->AddRectFilled(ImVec2(origin.x, origin.y + size.y - footerHeight), ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(218, 144, 34, 248), 18.0f, ImDrawFlags_RoundCornersBottom);
    drawList->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(250, 196, 64, 70), 18.0f, 0, 1.5f);

    ImGui::SetCursorPos(ImVec2(20.0f, 14.0f));
    ImGui::TextColored(ImVec4(0.98f, 0.98f, 0.98f, 1.0f), "ANIMATION PANEL");
    ImGui::SetCursorPos(ImVec2(20.0f, 43.0f));
    const char* headerLabel = "Categories";
    if (m_state.viewMode == kViewAnimations) {
        headerLabel = GetCategoryLabel(m_state.categoryIndex).c_str();
    } else if (m_state.viewMode == kViewSettings) {
        headerLabel = "Settings";
    }
    ImGui::TextColored(ImVec4(0.08f, 0.09f, 0.10f, 0.92f), "%s", headerLabel);
    ImGui::SameLine();
    if (m_state.viewMode == kViewAnimations) {
        const int page = (m_state.selectedResult / kVisibleRows) + 1;
        const int totalPages = m_state.filteredIndices.empty() ? 1 : ((static_cast<int>(m_state.filteredIndices.size()) - 1) / kVisibleRows) + 1;
        ImGui::SetCursorPosX(size.x - 110.0f);
        ImGui::TextColored(ImVec4(0.08f, 0.09f, 0.10f, 0.92f), "%d/%d", page, totalPages);
    } else {
        ImGui::SetCursorPosX(size.x - 74.0f);
        ImGui::TextColored(ImVec4(0.08f, 0.09f, 0.10f, 0.92f), "MENU");
    }

    ImGui::SetCursorPos(ImVec2(14.0f, 104.0f));
    ImGui::BeginChild("##MainList", ImVec2(size.x - 36.0f, size.y - headerHeight - footerHeight - 36.0f), false, ImGuiWindowFlags_NoScrollbar);
    if (m_state.viewMode == kViewCategories) {
        RenderCategoryMenu();
    } else if (m_state.viewMode == kViewSettings) {
        RenderSettingsMenu();
    } else {
        RenderAnimationMenu();
    }
    ImGui::EndChild();

    RenderFooter();
    ImGui::End();

    if (!m_state.visible) {
        m_state.wantsInputBlock = false;
    }
}

const AnimEntry* AnimPanelUI::GetSelectedEntry() const {
    if (m_state.filteredIndices.empty()) {
        return nullptr;
    }
    if (m_state.selectedResult < 0 || m_state.selectedResult >= static_cast<int>(m_state.filteredIndices.size())) {
        return nullptr;
    }
    return &m_catalog.Entries()[m_state.filteredIndices[static_cast<size_t>(m_state.selectedResult)]];
}

const std::string& AnimPanelUI::GetCategoryLabel(int index) const {
    static const std::string kFallbackCategory = "All";
    if (index < 0 || index >= static_cast<int>(m_categories.size())) {
        return kFallbackCategory;
    }
    return m_categories[static_cast<size_t>(index)];
}

bool AnimPanelUI::IsSettingsCategory(int index) const {
    return GetCategoryLabel(index) == "Settings";
}

void AnimPanelUI::TriggerPreviewForSelection() {
    const AnimEntry* selected = GetSelectedEntry();
    if (selected == nullptr || !m_callbacks.onPlay) {
        return;
    }

    m_callbacks.onPlay(*selected);
    m_catalog.MarkRecent(m_state.filteredIndices[static_cast<size_t>(m_state.selectedResult)]);
    m_state.recentsDirty = true;
    RefreshResults();
}

void AnimPanelUI::ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 18.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 10.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(6.0f, 2.0f);
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.06f, 0.08f, 0.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.10f, 0.14f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.19f, 0.26f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.97f, 0.97f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.76f, 0.78f, 0.82f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.95f, 0.78f, 0.14f, 0.20f);
    ImGui::GetIO().MouseDrawCursor = false;
}

void AnimPanelUI::RenderCategoryMenu() {
    for (int i = 0; i < static_cast<int>(m_categories.size()); ++i) {
        const bool selected = m_state.categoryCursor == i;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 itemSize(ImGui::GetContentRegionAvail().x, 28.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(start, ImVec2(start.x + itemSize.x, start.y + itemSize.y),
            selected ? IM_COL32(242, 191, 63, 245) : IM_COL32(39, 49, 75, 195), 10.0f);
        if (selected) {
            drawList->AddRectFilled(start, ImVec2(start.x + 7.0f, start.y + itemSize.y), IM_COL32(255, 255, 255, 255), 10.0f, ImDrawFlags_RoundCornersLeft);
        }

        const bool settingsCategory = IsSettingsCategory(i);
        const int count = settingsCategory ? 0 : static_cast<int>(m_catalog.Query("", GetCategoryLabel(i)).size());
        const ImU32 leftColor = selected ? IM_COL32(18, 18, 18, 255) : IM_COL32(250, 250, 247, 255);
        const ImU32 rightColor = selected ? IM_COL32(28, 28, 28, 225) : IM_COL32(250, 214, 70, 255);
        DrawBoldText(drawList, ImVec2(start.x + 18.0f, start.y + 3.5f), leftColor, GetCategoryLabel(i).c_str());
        if (settingsCategory) {
            DrawBoldText(drawList, ImVec2(start.x + itemSize.x - 58.0f, start.y + 3.5f), rightColor, "SET", 0.7f);
        } else {
            char countLabel[16];
            std::snprintf(countLabel, sizeof(countLabel), "%d", count);
            DrawBoldText(drawList, ImVec2(start.x + itemSize.x - 38.0f, start.y + 3.5f), rightColor, countLabel, 0.7f);
        }

        ImGui::Dummy(ImVec2(itemSize.x, itemSize.y));
    }
}

void AnimPanelUI::RenderAnimationMenu() {
    const int total = static_cast<int>(m_state.filteredIndices.size());
    int top = m_state.selectedResult - (kVisibleRows / 2);
    if (top < 0) {
        top = 0;
    }
    if (top > total - kVisibleRows) {
        top = total - kVisibleRows;
    }
    if (top < 0) {
        top = 0;
    }
    const int bottom = total < top + kVisibleRows ? total : top + kVisibleRows;

    for (int row = top; row < bottom; ++row) {
        const AnimEntry& entry = m_catalog.Entries()[m_state.filteredIndices[static_cast<size_t>(row)]];
        const bool selected = row == m_state.selectedResult;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 itemSize(ImGui::GetContentRegionAvail().x, 25.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(start, ImVec2(start.x + itemSize.x, start.y + itemSize.y),
            selected ? IM_COL32(242, 191, 63, 245) : IM_COL32(39, 49, 75, 195), 10.0f);
        if (selected) {
            drawList->AddRectFilled(start, ImVec2(start.x + 7.0f, start.y + itemSize.y), IM_COL32(255, 255, 255, 255), 10.0f, ImDrawFlags_RoundCornersLeft);
        }

        const ImU32 leftColor = selected ? IM_COL32(18, 18, 18, 255) : IM_COL32(250, 250, 247, 255);
        DrawBoldText(drawList, ImVec2(start.x + 16.0f, start.y + 1.5f), leftColor, entry.displayName.c_str(), 0.75f);

        ImGui::Dummy(ImVec2(itemSize.x, itemSize.y));
    }

    const AnimEntry* selected = GetSelectedEntry();
    if (selected != nullptr) {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.98f, 0.82f, 0.24f, 1.0f), "Selected");
        ImGui::Text("IFP : %s", selected->ifpFile.c_str());
        ImGui::Text("%s / %s", selected->block.c_str(), selected->animName.c_str());
    }
}

void AnimPanelUI::RenderSettingsMenu() {
    struct SettingRow {
        const char* label;
        bool enabled;
    };

    const SettingRow rows[2] = {
        { "Auto Play", m_state.autoPlayEnabled },
        { "Fast Mode", m_state.fastModeEnabled }
    };

    for (int i = 0; i < 2; ++i) {
        const bool selected = m_state.settingsCursor == i;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 itemSize(ImGui::GetContentRegionAvail().x, 32.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(start, ImVec2(start.x + itemSize.x, start.y + itemSize.y),
            selected ? IM_COL32(242, 191, 63, 245) : IM_COL32(39, 49, 75, 195), 10.0f);
        if (selected) {
            drawList->AddRectFilled(start, ImVec2(start.x + 7.0f, start.y + itemSize.y), IM_COL32(255, 255, 255, 255), 10.0f, ImDrawFlags_RoundCornersLeft);
        }

        const ImU32 leftColor = selected ? IM_COL32(18, 18, 18, 255) : IM_COL32(250, 250, 247, 255);
        const ImU32 rightColor = selected ? IM_COL32(28, 28, 28, 225) : IM_COL32(250, 214, 70, 255);
        DrawBoldText(drawList, ImVec2(start.x + 16.0f, start.y + 4.0f), leftColor, rows[i].label, 0.75f);
        DrawBoldText(drawList, ImVec2(start.x + itemSize.x - 58.0f, start.y + 4.0f), rightColor, rows[i].enabled ? "ON" : "OFF", 0.7f);

        ImGui::Dummy(ImVec2(itemSize.x, itemSize.y));
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::TextWrapped("Auto Play previews the current animation automatically. Fast Mode lets you hold 8 or 2 to browse quickly.");
}

void AnimPanelUI::RenderFooter() {
    const ImVec2 size = ImGui::GetWindowSize();
    const bool isFailure = m_state.statusLine.find("failed") != std::string::npos ||
        m_state.statusLine.find("fault") != std::string::npos ||
        m_state.statusLine.find("Known fault") != std::string::npos;

    if (!m_state.statusLine.empty()) {
        ImGui::SetCursorPos(ImVec2(14.0f, size.y - 94.0f));
        ImGui::TextColored(isFailure ? ImVec4(0.95f, 0.30f, 0.25f, 1.0f) : ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "%s", m_state.statusLine.c_str());
    }

    ImGui::SetCursorPos(ImVec2(14.0f, size.y - 64.0f));
    if (m_state.viewMode == kViewCategories) {
        ImGui::TextColored(ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "8 UP   2 DOWN   5 OPEN");
        ImGui::TextColored(ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "F8 / ESC CLOSE");
    } else if (m_state.viewMode == kViewSettings) {
        ImGui::TextColored(ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "8/2 SELECT   5 TOGGLE");
        ImGui::TextColored(ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "BACK RETURN   ESC CLOSE");
    } else {
        ImGui::TextColored(ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "8 UP  2 DOWN  4/6 PAGE  5 PLAY");
        ImGui::TextColored(ImVec4(0.09f, 0.09f, 0.09f, 1.0f), "BACK RETURN   ESC CLOSE");
    }
}

} // namespace animpanel
