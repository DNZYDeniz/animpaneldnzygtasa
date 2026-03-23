#pragma once

#include "AnimCatalog.h"

#include <functional>
#include <cstdint>
#include <string>
#include <vector>

namespace animpanel {

struct AnimPanelCallbacks {
    std::function<void(const AnimEntry&)> onPlay;
    std::function<void(const AnimEntry&)> onCopy;
    std::function<void()> onStop;
};

struct AnimPanelState {
    bool visible = false;
    bool wantsInputBlock = false;
    bool favoritesDirty = false;
    bool recentsDirty = false;
    bool settingsDirty = false;
    bool autoPlayEnabled = false;
    bool fastModeEnabled = false;
    int selectedResult = 0;
    int categoryIndex = 0;
    int categoryCursor = 0;
    int settingsCursor = 0;
    int viewMode = 0;
    void* favoriteIconTexture = nullptr;
    float favoriteIconWidth = 0.0f;
    float favoriteIconHeight = 0.0f;
    std::string toastText;
    std::uint32_t toastStartedAt = 0;
    std::uint32_t toastDurationMs = 0;
    std::vector<size_t> filteredIndices;
    std::string statusLine;
};

class AnimPanelUI {
public:
    AnimPanelUI(AnimCatalog& catalog, AnimPanelState& state, AnimPanelCallbacks callbacks);

    void RefreshResults();
    void Render();
    void HandleKeyboard();

private:
    const AnimEntry* GetSelectedEntry() const;
    const std::string& GetCategoryLabel(int index) const;
    bool IsSettingsCategory(int index) const;
    void TriggerPreviewForSelection();
    void ToggleFavoriteForSelection();
    void ApplyStyle();
    void RenderCategoryMenu();
    void RenderAnimationMenu();
    void RenderSelectionDetails();
    void RenderSettingsMenu();
    void RenderFavoriteToast();
    void RenderFooter();

    AnimCatalog& m_catalog;
    AnimPanelState& m_state;
    AnimPanelCallbacks m_callbacks;
    std::vector<std::string> m_categories;
};

} // namespace animpanel
