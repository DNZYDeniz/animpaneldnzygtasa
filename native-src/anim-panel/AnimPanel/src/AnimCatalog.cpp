#include "AnimCatalog.h"

#include "FlatJson.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>
#include <sstream>

namespace animpanel {

bool AnimCatalog::LoadCatalog(const std::string& path, std::string& error) {
    std::string content = ReadTextFile(path, error);
    if (!error.empty()) {
        return false;
    }

    std::vector<FlatJsonObject> objects;
    FlatJsonParser parser(std::move(content));
    if (!parser.ParseArrayOfObjects(objects, error)) {
        return false;
    }

    m_entries.clear();
    m_entries.reserve(objects.size());

    for (const FlatJsonObject& object : objects) {
        AnimEntry entry;
        entry.id = object.strings.at("id");
        entry.ifpFile = object.strings.at("ifp_file");
        entry.block = object.strings.at("block");
        entry.animName = object.strings.at("anim_name");
        entry.displayName = object.strings.at("display_name");
        entry.category = object.strings.at("category");
        entry.notes = object.strings.at("notes");
        entry.tags = object.arrays.at("tags");
        entry.loopDefault = object.bools.at("loop_default");
        entry.pedFlag = object.bools.at("ped_flag");
        entry.lockF = object.bools.at("lock_f");
        entry.poseFlag = object.bools.at("pose_flag");
        RebuildSearchHaystack(entry);
        m_entries.push_back(std::move(entry));
    }

    RebuildMenuCategories();
    return true;
}

bool AnimCatalog::LoadFavorites(const std::string& path, std::string& error) {
    std::unordered_set<std::string> favorites;
    if (!ReadStringIdArray(path, favorites, error)) {
        return false;
    }

    for (AnimEntry& entry : m_entries) {
        entry.favorite = favorites.find(entry.id) != favorites.end();
    }

    return true;
}

bool AnimCatalog::LoadRecents(const std::string& path, std::string& error) {
    std::unordered_set<std::string> recents;
    if (!ReadStringIdArray(path, recents, error)) {
        return false;
    }

    for (AnimEntry& entry : m_entries) {
        entry.recent = recents.find(entry.id) != recents.end();
    }

    return true;
}

bool AnimCatalog::SaveFavorites(const std::string& path, std::string& error) const {
    std::vector<std::string> values;
    for (const AnimEntry& entry : m_entries) {
        if (entry.favorite) {
            values.push_back(entry.id);
        }
    }
    return WriteStringIdArray(path, values, error);
}

bool AnimCatalog::SaveRecents(const std::string& path, std::string& error) const {
    std::vector<std::string> values;
    for (const AnimEntry& entry : m_entries) {
        if (entry.recent) {
            values.push_back(entry.id);
        }
    }
    return WriteStringIdArray(path, values, error);
}

std::vector<size_t> AnimCatalog::Query(const std::string& searchText, const std::string& category) const {
    std::vector<size_t> results;
    const std::string loweredSearch = ToLower(searchText);
    const bool filterFavorites = category == "Favorites";
    const bool filterRecent = category == "Recent";

    for (size_t i = 0; i < m_entries.size(); ++i) {
        const AnimEntry& entry = m_entries[i];
        if (filterFavorites && !entry.favorite) {
            continue;
        }
        if (filterRecent && !entry.recent) {
            continue;
        }
        if (!filterFavorites && !filterRecent && category != "All" && entry.category != category) {
            continue;
        }
        if (!loweredSearch.empty() && entry.searchHaystack.find(loweredSearch) == std::string::npos) {
            continue;
        }
        results.push_back(i);
    }

    return results;
}

void AnimCatalog::ToggleFavorite(size_t index) {
    if (index < m_entries.size()) {
        m_entries[index].favorite = !m_entries[index].favorite;
    }
}

void AnimCatalog::MarkRecent(size_t index) {
    if (index >= m_entries.size()) {
        return;
    }

    for (AnimEntry& entry : m_entries) {
        entry.recent = false;
    }
    m_entries[index].recent = true;
}

std::string AnimCatalog::ReadTextFile(const std::string& path, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Failed to open file: " + path;
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }
    return content;
}

std::string AnimCatalog::ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string AnimCatalog::EscapeJson(const std::string& value) {
    std::string output;
    output.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
        case '\\': output += "\\\\"; break;
        case '"': output += "\\\""; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default: output.push_back(ch); break;
        }
    }
    return output;
}

bool AnimCatalog::ReadStringIdArray(const std::string& path, std::unordered_set<std::string>& out, std::string& error) {
    std::string content = ReadTextFile(path, error);
    if (!error.empty()) {
        return false;
    }

    std::vector<std::string> values;
    FlatJsonParser parser(std::move(content));
    if (!parser.ParseStringArray(values, error)) {
        return false;
    }

    out.clear();
    out.insert(values.begin(), values.end());
    return true;
}

bool AnimCatalog::WriteStringIdArray(const std::string& path, const std::vector<std::string>& values, std::string& error) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Failed to write file: " + path;
        return false;
    }

    output << "[\n";
    for (size_t i = 0; i < values.size(); ++i) {
        output << "  \"" << EscapeJson(values[i]) << "\"";
        if (i + 1 < values.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "]";
    return true;
}

void AnimCatalog::RebuildMenuCategories() {
    static const char* kPreferredCategories[] = {
        "Idle",
        "Social",
        "Gestures",
        "Sitting",
        "Leaning",
        "Walking",
        "Running",
        "Dancing",
        "Fighting",
        "Weapons",
        "Gang",
        "Smoking",
        "Eating & Drinking",
        "Indoor & Chores",
        "Exercise",
        "Injury & Death",
        "Adult",
        "Misc"
    };

    m_menuCategories.clear();
    m_menuCategories.push_back("All");
    m_menuCategories.push_back("Favorites");
    m_menuCategories.push_back("Recent");

    std::unordered_set<std::string> seen;
    for (const AnimEntry& entry : m_entries) {
        seen.insert(entry.category);
    }

    for (const char* category : kPreferredCategories) {
        if (seen.find(category) != seen.end()) {
            m_menuCategories.push_back(category);
            seen.erase(category);
        }
    }

    std::vector<std::string> leftovers(seen.begin(), seen.end());
    std::sort(leftovers.begin(), leftovers.end());
    for (const std::string& category : leftovers) {
        m_menuCategories.push_back(category);
    }
}

void AnimCatalog::RebuildSearchHaystack(AnimEntry& entry) {
    std::ostringstream buffer;
    buffer << entry.displayName << ' '
           << entry.block << ' '
           << entry.animName << ' '
           << entry.ifpFile << ' '
           << entry.category << ' '
           << entry.notes << ' ';

    for (const std::string& tag : entry.tags) {
        buffer << tag << ' ';
    }

    entry.searchHaystack = ToLower(buffer.str());
}

} // namespace animpanel
