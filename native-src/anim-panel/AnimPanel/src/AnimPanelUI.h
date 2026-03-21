#pragma once

#include "AnimCatalog.h"

#include <functional>
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
    int selectedResult = 0;
    int categoryIndex = 0;
    int categoryCursor = 0;
    int viewMode = 0;
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
    void ApplyStyle();
    void RenderCategoryMenu();
    void RenderAnimationMenu();
    void RenderFooter();

    AnimCatalog& m_catalog;
    AnimPanelState& m_state;
    AnimPanelCallbacks m_callbacks;
};

} // namespace animpanel
