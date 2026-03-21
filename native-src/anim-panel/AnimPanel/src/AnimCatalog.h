#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace animpanel {

struct AnimEntry {
    std::string id;
    std::string ifpFile;
    std::string block;
    std::string animName;
    std::string displayName;
    std::string category;
    std::vector<std::string> tags;
    bool loopDefault = false;
    bool poseFlag = false;
    std::string notes;
    bool favorite = false;
    bool recent = false;
    std::string searchHaystack;
};

class AnimCatalog {
public:
    bool LoadCatalog(const std::string& path, std::string& error);
    bool LoadFavorites(const std::string& path, std::string& error);
    bool LoadRecents(const std::string& path, std::string& error);
    bool SaveFavorites(const std::string& path, std::string& error) const;
    bool SaveRecents(const std::string& path, std::string& error) const;

    std::vector<size_t> Query(const std::string& searchText, const std::string& category) const;
    const std::vector<AnimEntry>& Entries() const { return m_entries; }

    void ToggleFavorite(size_t index);
    void MarkRecent(size_t index);

private:
    static std::string ReadTextFile(const std::string& path, std::string& error);
    static std::string ToLower(std::string value);
    static std::string EscapeJson(const std::string& value);
    static bool ReadStringIdArray(const std::string& path, std::unordered_set<std::string>& out, std::string& error);
    static bool WriteStringIdArray(const std::string& path, const std::vector<std::string>& values, std::string& error);
    void RebuildSearchHaystack(AnimEntry& entry);

    std::vector<AnimEntry> m_entries;
};

} // namespace animpanel
