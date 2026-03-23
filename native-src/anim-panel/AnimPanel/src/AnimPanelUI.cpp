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
constexpr int kAnimationVisibleRows = 14;
constexpr int kCategoryVisibleRows = 18;
constexpr DWORD kFastModeInitialDelayMs = 220;
constexpr DWORD kFastModeRepeatDelayMs = 55;
constexpr float kPanelWidth = 380.0f;
constexpr float kHeaderHeight = 60.0f;
constexpr float kBodyPadding = 5.0f;
constexpr float kRowHeight = 31.0f;
constexpr float kRowGap = 3.0f;
constexpr float kAnimationBodyHeight = (kBodyPadding * 2.0f) + (kAnimationVisibleRows * (kRowHeight + kRowGap)) - kRowGap;
constexpr float kCategoryBodyHeight = (kBodyPadding * 2.0f) + (kCategoryVisibleRows * (kRowHeight + kRowGap)) - kRowGap;
constexpr float kSelectionDetailsHeight = 112.0f;
constexpr float kFooterHeightAnimation = 204.0f;
constexpr float kFooterHeightCompact = 124.0f;
constexpr float kPanelScreenMargin = 18.0f;
constexpr DWORD kToastDurationMs = 1650;

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
    (void)offset;
    drawList->AddText(pos, color, text);
}

struct FooterHint {
    const char* key;
    const char* label;
    ImU32 keyColor;
    ImU32 bodyColor;
};

void DrawFooterHint(ImDrawList* drawList, const ImVec2& start, const FooterHint& hint) {
    const ImVec2 keyTextSize = ImGui::CalcTextSize(hint.key);
    const ImVec2 labelTextSize = ImGui::CalcTextSize(hint.label);
    const float keyWidth = keyTextSize.x + 16.0f;
    const float labelWidth = labelTextSize.x + 16.0f;
    const float totalWidth = keyWidth + labelWidth;
    const float height = ImGui::GetFontSize() + 8.0f;

    drawList->AddRectFilled(start, ImVec2(start.x + totalWidth, start.y + height), hint.bodyColor, 4.0f);
    drawList->AddRectFilled(start, ImVec2(start.x + keyWidth, start.y + height), IM_COL32(0, 0, 0, 150), 4.0f, ImDrawFlags_RoundCornersLeft);

    DrawBoldText(drawList, ImVec2(start.x + 8.0f, start.y + 4.0f), hint.keyColor, hint.key, 0.0f);
    DrawBoldText(drawList, ImVec2(start.x + keyWidth + 8.0f, start.y + 4.0f), IM_COL32(255, 255, 255, 255), hint.label, 0.0f);
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
        m_state.selectedResult -= kAnimationVisibleRows;
        if (m_state.selectedResult < 0) {
            m_state.selectedResult = 0;
        }
        selectionChanged = true;
    }

    if (ConsumeAnyKeyPress({ VK_NUMPAD6, VK_RIGHT }) && m_state.viewMode == kViewAnimations && !m_state.filteredIndices.empty()) {
        m_state.selectedResult += kAnimationVisibleRows;
        if (m_state.selectedResult >= static_cast<int>(m_state.filteredIndices.size())) {
            m_state.selectedResult = static_cast<int>(m_state.filteredIndices.size() - 1);
        }
        selectionChanged = true;
    }

    if (ConsumeAnyKeyPress({ VK_F7, '7', VK_NUMPAD7 }) && m_state.viewMode == kViewAnimations) {
        ToggleFavoriteForSelection();
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
    const bool isAnimationMode = m_state.viewMode == kViewAnimations;
    const bool isCategoryMode = m_state.viewMode == kViewCategories;
    const float infoHeight = isAnimationMode ? kSelectionDetailsHeight : 0.0f;
    const float footerHeight = isAnimationMode ? kFooterHeightAnimation : kFooterHeightCompact;
    const float bodyHeight = isCategoryMode ? kCategoryBodyHeight : kAnimationBodyHeight;
    float panelHeight = kHeaderHeight + bodyHeight + infoHeight + footerHeight;
    const ImVec2 panelSize(kPanelWidth, panelHeight);

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
    ImGui::SetWindowFontScale(1.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float bodyBottom = origin.y + kHeaderHeight + bodyHeight + infoHeight;

    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + kHeaderHeight), IM_COL32(227, 156, 31, 255), 12.0f, ImDrawFlags_RoundCornersTop);
    drawList->AddRectFilled(ImVec2(origin.x, origin.y + kHeaderHeight), ImVec2(origin.x + size.x, bodyBottom), IM_COL32(13, 17, 26, 255));
    drawList->AddRectFilled(ImVec2(origin.x, bodyBottom), ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(227, 156, 31, 255), 12.0f, ImDrawFlags_RoundCornersBottom);

    ImGui::SetCursorPos(ImVec2(15.0f, 12.0f));
    ImGui::TextColored(ImVec4(0.98f, 0.98f, 0.98f, 1.0f), "ANIMATION PANEL");
    ImGui::SetCursorPos(ImVec2(15.0f, 36.0f));
    const char* headerLabel = "Categories";
    if (m_state.viewMode == kViewAnimations) {
        headerLabel = GetCategoryLabel(m_state.categoryIndex).c_str();
    } else if (m_state.viewMode == kViewSettings) {
        headerLabel = "Settings";
    }
    ImGui::TextColored(ImVec4(0.08f, 0.09f, 0.10f, 0.92f), "%s", headerLabel);
    ImGui::SameLine();
    if (m_state.viewMode == kViewAnimations) {
        const int page = (m_state.selectedResult / kAnimationVisibleRows) + 1;
        const int totalPages = m_state.filteredIndices.empty() ? 1 : ((static_cast<int>(m_state.filteredIndices.size()) - 1) / kAnimationVisibleRows) + 1;
        ImGui::SetCursorPosX(size.x - 15.0f - ImGui::CalcTextSize("12/12").x);
        ImGui::TextColored(ImVec4(0.08f, 0.09f, 0.10f, 0.92f), "%d/%d", page, totalPages);
    } else {
        ImGui::SetCursorPosX(size.x - 15.0f - ImGui::CalcTextSize("MENU").x);
        ImGui::TextColored(ImVec4(0.08f, 0.09f, 0.10f, 0.92f), "MENU");
    }
    drawList->AddLine(ImVec2(origin.x, origin.y + kHeaderHeight - 2.0f), ImVec2(origin.x + size.x, origin.y + kHeaderHeight - 2.0f), IM_COL32(255, 255, 255, 100), 1.0f);

    ImGui::SetCursorPos(ImVec2(10.0f, kHeaderHeight + kBodyPadding));
    ImGui::BeginChild("##MainList", ImVec2(360.0f, bodyHeight), false, ImGuiWindowFlags_NoScrollbar);
    if (m_state.viewMode == kViewCategories) {
        RenderCategoryMenu();
    } else if (m_state.viewMode == kViewSettings) {
        RenderSettingsMenu();
    } else {
        RenderAnimationMenu();
    }
    ImGui::EndChild();

    if (isAnimationMode) {
        RenderSelectionDetails();
    }

    RenderFavoriteToast();
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

void AnimPanelUI::ToggleFavoriteForSelection() {
    const AnimEntry* selected = GetSelectedEntry();
    if (selected == nullptr) {
        return;
    }

    const size_t entryIndex = m_state.filteredIndices[static_cast<size_t>(m_state.selectedResult)];
    const bool wasFavorite = selected->favorite;
    m_catalog.ToggleFavorite(entryIndex);
    m_state.favoritesDirty = true;
    m_state.statusLine = wasFavorite ? "Removed from favorites." : "Added to favorites.";
    m_state.toastText = wasFavorite ? "Favorilerden kaldirildi" : "Favorilere eklendi";
    m_state.toastStartedAt = GetTickCount();
    m_state.toastDurationMs = kToastDurationMs;
    RefreshResults();

    if (m_state.filteredIndices.empty()) {
        m_state.selectedResult = 0;
    } else if (m_state.selectedResult >= static_cast<int>(m_state.filteredIndices.size())) {
        m_state.selectedResult = static_cast<int>(m_state.filteredIndices.size() - 1);
    }
}

void AnimPanelUI::ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 18.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(0.0f, 3.0f);
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.06f, 0.08f, 0.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.10f, 0.14f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.14f, 0.21f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.97f, 0.97f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.76f, 0.78f, 0.82f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.95f, 0.78f, 0.14f, 0.20f);
    ImGui::GetIO().MouseDrawCursor = false;
}

void AnimPanelUI::RenderCategoryMenu() {
    const int total = static_cast<int>(m_categories.size());
    int top = m_state.categoryCursor - (kCategoryVisibleRows / 2);
    if (top < 0) {
        top = 0;
    }
    if (top > total - kCategoryVisibleRows) {
        top = total - kCategoryVisibleRows;
    }
    if (top < 0) {
        top = 0;
    }
    const int bottom = total < top + kCategoryVisibleRows ? total : top + kCategoryVisibleRows;

    for (int i = top; i < bottom; ++i) {
        const bool selected = m_state.categoryCursor == i;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 itemSize(360.0f, kRowHeight);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(start, ImVec2(start.x + itemSize.x, start.y + itemSize.y),
            selected ? IM_COL32(227, 196, 63, 255) : IM_COL32(30, 36, 54, 255), 8.0f);

        const bool settingsCategory = IsSettingsCategory(i);
        const int count = settingsCategory ? 0 : static_cast<int>(m_catalog.Query("", GetCategoryLabel(i)).size());
        const ImU32 leftColor = selected ? IM_COL32(18, 18, 18, 255) : IM_COL32(250, 250, 247, 255);
        const ImU32 rightColor = selected ? IM_COL32(28, 28, 28, 225) : IM_COL32(250, 214, 70, 255);
        DrawBoldText(drawList, ImVec2(start.x + 14.0f, start.y + 8.0f), leftColor, GetCategoryLabel(i).c_str());
        if (settingsCategory) {
            DrawBoldText(drawList, ImVec2(start.x + itemSize.x - 42.0f, start.y + 8.0f), rightColor, "SET", 0.0f);
        } else {
            char countLabel[16];
            std::snprintf(countLabel, sizeof(countLabel), "%d", count);
            const ImVec2 countSize = ImGui::CalcTextSize(countLabel);
            DrawBoldText(drawList, ImVec2(start.x + itemSize.x - countSize.x - 16.0f, start.y + 8.0f), rightColor, countLabel, 0.0f);
        }

        ImGui::Dummy(ImVec2(itemSize.x, itemSize.y));
    }
}

void AnimPanelUI::RenderAnimationMenu() {
    const int total = static_cast<int>(m_state.filteredIndices.size());
    int top = m_state.selectedResult - (kAnimationVisibleRows / 2);
    if (top < 0) {
        top = 0;
    }
    if (top > total - kAnimationVisibleRows) {
        top = total - kAnimationVisibleRows;
    }
    if (top < 0) {
        top = 0;
    }
    const int bottom = total < top + kAnimationVisibleRows ? total : top + kAnimationVisibleRows;

    for (int row = top; row < bottom; ++row) {
        const AnimEntry& entry = m_catalog.Entries()[m_state.filteredIndices[static_cast<size_t>(row)]];
        const bool selected = row == m_state.selectedResult;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 itemSize(360.0f, kRowHeight);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(start, ImVec2(start.x + itemSize.x, start.y + itemSize.y),
            selected ? IM_COL32(227, 196, 63, 255) : IM_COL32(30, 36, 54, 255), 8.0f);

        const ImU32 leftColor = selected ? IM_COL32(18, 18, 18, 255) : IM_COL32(250, 250, 247, 255);
        DrawBoldText(drawList, ImVec2(start.x + 13.0f, start.y + 8.0f), leftColor, entry.displayName.c_str(), 0.0f);

        if (entry.favorite) {
            if (m_state.favoriteIconTexture != nullptr) {
                const float iconWidth = m_state.favoriteIconWidth > 0.0f ? m_state.favoriteIconWidth : 56.0f;
                const float iconHeight = m_state.favoriteIconHeight > 0.0f ? m_state.favoriteIconHeight : 29.0f;
                const float iconX = start.x + itemSize.x - iconWidth - 8.0f;
                const float iconY = start.y + ((itemSize.y - iconHeight) * 0.5f);
                drawList->AddImage(
                    m_state.favoriteIconTexture,
                    ImVec2(iconX, iconY),
                    ImVec2(iconX + iconWidth, iconY + iconHeight),
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    IM_COL32(255, 255, 255, 255));
            } else {
                DrawBoldText(drawList, ImVec2(start.x + itemSize.x - 22.0f, start.y + 8.0f),
                    selected ? IM_COL32(28, 28, 28, 255) : IM_COL32(250, 214, 70, 255), "*", 0.0f);
            }
        }

        ImGui::Dummy(ImVec2(itemSize.x, itemSize.y));
    }
}

void AnimPanelUI::RenderSelectionDetails() {
    const AnimEntry* selected = GetSelectedEntry();
    if (selected == nullptr) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float footerTop = origin.y + size.y - kFooterHeightAnimation;
    const float detailsTop = footerTop - kSelectionDetailsHeight;
    const float lineStartX = origin.x + 10.0f;
    const float lineEndX = origin.x + 370.0f;

    drawList->AddLine(ImVec2(lineStartX, detailsTop + 12.0f), ImVec2(lineEndX, detailsTop + 12.0f), IM_COL32(227, 156, 31, 70), 1.0f);

    ImGui::SetCursorScreenPos(ImVec2(origin.x + 15.0f, detailsTop + 28.0f));
    ImGui::TextColored(ImVec4(0.89f, 0.61f, 0.12f, 1.0f), "Selected");
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 15.0f, detailsTop + 54.0f));
    ImGui::Text("IFP : %s", selected->ifpFile.c_str());
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 15.0f, detailsTop + 78.0f));
    ImGui::Text("%s / %s", selected->block.c_str(), selected->animName.c_str());
}

void AnimPanelUI::RenderFavoriteToast() {
    if (m_state.toastText.empty() || m_state.toastDurationMs == 0) {
        return;
    }

    const DWORD now = GetTickCount();
    const DWORD elapsed = now - m_state.toastStartedAt;
    if (elapsed >= m_state.toastDurationMs) {
        m_state.toastText.clear();
        m_state.toastStartedAt = 0;
        m_state.toastDurationMs = 0;
        return;
    }

    float alpha = 1.0f;
    if (elapsed < 160) {
        alpha = static_cast<float>(elapsed) / 160.0f;
    } else if (elapsed > m_state.toastDurationMs - 420) {
        alpha = static_cast<float>(m_state.toastDurationMs - elapsed) / 420.0f;
    }

    if (alpha < 0.0f) {
        alpha = 0.0f;
    }
    if (alpha > 1.0f) {
        alpha = 1.0f;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const ImVec2 textSize = ImGui::CalcTextSize(m_state.toastText.c_str());
    const float width = textSize.x + 36.0f;
    const float height = 34.0f;
    float x = origin.x + ((size.x - width) * 0.5f);
    float y = origin.y - height - 10.0f;
    if (y < viewport->WorkPos.y + 8.0f) {
        y = viewport->WorkPos.y + 8.0f;
    }

    const ImU32 bgColor = IM_COL32(227, 156, 31, static_cast<int>(220.0f * alpha));
    const ImU32 borderColor = IM_COL32(255, 220, 120, static_cast<int>(110.0f * alpha));
    const ImU32 textColor = IM_COL32(10, 10, 10, static_cast<int>(255.0f * alpha));
    const ImU32 shadowColor = IM_COL32(0, 0, 0, static_cast<int>(70.0f * alpha));
    drawList->AddRectFilled(ImVec2(x + 2.0f, y + 2.0f), ImVec2(x + width + 2.0f, y + height + 2.0f), shadowColor, 9.0f);
    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), bgColor, 8.0f);
    drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), borderColor, 8.0f, 0, 1.0f);
    DrawBoldText(drawList, ImVec2(x + 18.0f, y + 8.0f), textColor, m_state.toastText.c_str(), 0.0f);
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
    const bool isAnimationMode = m_state.viewMode == kViewAnimations;
    const bool isCompactMode = m_state.viewMode == kViewCategories || m_state.viewMode == kViewSettings;
    const ImVec2 size = ImGui::GetWindowSize();
    const bool isFailure = m_state.statusLine.find("failed") != std::string::npos ||
        m_state.statusLine.find("fault") != std::string::npos ||
        m_state.statusLine.find("Known fault") != std::string::npos;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const float footerHeight = isAnimationMode ? kFooterHeightAnimation : kFooterHeightCompact;
    const float footerTop = origin.y + size.y - footerHeight;
    const float lineX = origin.x + 15.0f;
    const float lineY = footerTop + (isAnimationMode ? 24.0f : 20.0f);

    std::string lead;
    std::string badge;
    if (isAnimationMode) {
        const AnimEntry* selected = GetSelectedEntry();
        if (selected != nullptr) {
            const std::string previewText = selected->block + "/" + selected->animName;
            if (isFailure && !m_state.statusLine.empty()) {
                lead = m_state.statusLine;
            } else {
                lead = "Previewing";
                badge = previewText;
            }
        }
    } else if (!isCompactMode && !m_state.statusLine.empty()) {
        lead = m_state.statusLine;
    }

    if (!lead.empty()) {
        const ImU32 leadColor = isFailure ? IM_COL32(140, 10, 10, 255) : IM_COL32(14, 14, 14, 255);
        DrawBoldText(drawList, ImVec2(lineX, lineY), leadColor, lead.c_str(), 0.65f);
        if (!badge.empty()) {
            const ImVec2 leadSize = ImGui::CalcTextSize(lead.c_str());
            const ImVec2 badgePos(lineX + leadSize.x + 5.0f, lineY - 1.0f);
            const ImVec2 badgeTextSize = ImGui::CalcTextSize(badge.c_str());
            drawList->AddRectFilled(
                badgePos,
                ImVec2(badgePos.x + badgeTextSize.x + 10.0f, badgePos.y + ImGui::GetFontSize() + 4.0f),
                IM_COL32(0, 0, 0, 50),
                8.0f);
            DrawBoldText(drawList, ImVec2(badgePos.x + 5.0f, badgePos.y + 2.0f), IM_COL32(14, 14, 14, 255), badge.c_str(), 0.0f);
        }
    }

    std::vector<FooterHint> hints;
    if (m_state.viewMode == kViewCategories) {
        hints = {
            { "8", "UP", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "2", "DOWN", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "5", "OPEN", IM_COL32(74, 222, 128, 255), IM_COL32(13, 17, 26, 255) },
            { "ESC", "CLOSE", IM_COL32(246, 116, 116, 255), IM_COL32(118, 24, 28, 210) }
        };
    } else if (m_state.viewMode == kViewSettings) {
        hints = {
            { "8", "UP", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "2", "DOWN", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "5", "TOGGLE", IM_COL32(74, 222, 128, 255), IM_COL32(13, 17, 26, 255) },
            { "BACK", "RETURN", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "ESC", "CLOSE", IM_COL32(246, 116, 116, 255), IM_COL32(118, 24, 28, 210) }
        };
    } else {
        hints = {
            { "8", "UP", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "2", "DOWN", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "4 / 6", "PAGE", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "5", "PLAY", IM_COL32(74, 222, 128, 255), IM_COL32(13, 17, 26, 255) },
            { "BACK", "RETURN", IM_COL32(250, 214, 70, 255), IM_COL32(13, 17, 26, 255) },
            { "F7", "FAVORITES", IM_COL32(96, 165, 250, 255), IM_COL32(13, 17, 26, 255) },
            { "ESC", "CLOSE", IM_COL32(246, 116, 116, 255), IM_COL32(118, 24, 28, 210) }
        };
    }

    std::vector<std::vector<FooterHint>> rows;
    if (m_state.viewMode == kViewCategories) {
        rows = {
            { hints[0], hints[1], hints[2] },
            { hints[3] }
        };
    } else if (m_state.viewMode == kViewSettings) {
        rows = {
            { hints[0], hints[1], hints[2] },
            { hints[3], hints[4] }
        };
    } else {
        rows = {
            { hints[0], hints[1], hints[2] },
            { hints[3], hints[5] },
            { hints[4], hints[6] }
        };
    }

    const float gapX = 6.0f;
    float cursorY = footerTop + (isAnimationMode ? 80.0f : 40.0f);
    for (const std::vector<FooterHint>& row : rows) {
        float rowWidth = 0.0f;
        std::vector<float> widths;
        widths.reserve(row.size());
        for (const FooterHint& hint : row) {
            const float keyWidth = ImGui::CalcTextSize(hint.key).x + 16.0f;
            const float labelWidth = ImGui::CalcTextSize(hint.label).x + 16.0f;
            const float totalWidth = keyWidth + labelWidth;
            widths.push_back(totalWidth);
            rowWidth += totalWidth;
        }
        for (size_t i = 0; i < row.size(); ++i) {
            if (i + 1 < row.size()) {
                rowWidth += gapX;
            }
        }

        float cursorX = origin.x + ((size.x - rowWidth) * 0.5f);
        for (size_t i = 0; i < row.size(); ++i) {
            DrawFooterHint(drawList, ImVec2(cursorX, cursorY), row[i]);
            cursorX += widths[i] + gapX;
        }
        cursorY += ImGui::GetFontSize() + 20.0f;
    }
}

} // namespace animpanel
