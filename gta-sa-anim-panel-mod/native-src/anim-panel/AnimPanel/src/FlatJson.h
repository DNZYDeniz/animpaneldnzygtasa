#pragma once

#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

namespace animpanel {

struct FlatJsonObject {
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, bool> bools;
    std::unordered_map<std::string, std::vector<std::string>> arrays;
};

class FlatJsonParser {
public:
    explicit FlatJsonParser(std::string text) : m_text(std::move(text)) {}

    bool ParseArrayOfObjects(std::vector<FlatJsonObject>& out, std::string& error) {
        SkipWhitespace();
        if (!Consume('[')) {
            error = "JSON root is not an array.";
            return false;
        }

        SkipWhitespace();
        while (!End() && Peek() != ']') {
            FlatJsonObject object;
            if (!ParseObject(object, error)) {
                return false;
            }
            out.push_back(std::move(object));
            SkipWhitespace();
            if (Peek() == ',') {
                Advance();
                SkipWhitespace();
            }
        }

        if (!Consume(']')) {
            error = "JSON array is missing closing bracket.";
            return false;
        }

        return true;
    }

    bool ParseStringArray(std::vector<std::string>& out, std::string& error) {
        SkipWhitespace();
        if (!Consume('[')) {
            error = "JSON root is not a string array.";
            return false;
        }

        SkipWhitespace();
        while (!End() && Peek() != ']') {
            std::string value;
            if (!ParseString(value, error)) {
                return false;
            }
            out.push_back(std::move(value));
            SkipWhitespace();
            if (Peek() == ',') {
                Advance();
                SkipWhitespace();
            }
        }

        if (!Consume(']')) {
            error = "String array is missing closing bracket.";
            return false;
        }

        return true;
    }

private:
    bool ParseObject(FlatJsonObject& out, std::string& error) {
        SkipWhitespace();
        if (!Consume('{')) {
            error = "Object is missing opening brace.";
            return false;
        }

        SkipWhitespace();
        while (!End() && Peek() != '}') {
            std::string key;
            if (!ParseString(key, error)) {
                return false;
            }

            SkipWhitespace();
            if (!Consume(':')) {
                error = "Object is missing ':' after key.";
                return false;
            }

            SkipWhitespace();
            if (Peek() == '"') {
                std::string value;
                if (!ParseString(value, error)) {
                    return false;
                }
                out.strings[key] = std::move(value);
            } else if (Peek() == '[') {
                std::vector<std::string> values;
                if (!ParseStringArray(values, error)) {
                    return false;
                }
                out.arrays[key] = std::move(values);
            } else if (StartsWith("true")) {
                m_cursor += 4;
                out.bools[key] = true;
            } else if (StartsWith("false")) {
                m_cursor += 5;
                out.bools[key] = false;
            } else {
                error = "Unsupported JSON value in flat object.";
                return false;
            }

            SkipWhitespace();
            if (Peek() == ',') {
                Advance();
                SkipWhitespace();
            }
        }

        if (!Consume('}')) {
            error = "Object is missing closing brace.";
            return false;
        }

        return true;
    }

    bool ParseString(std::string& out, std::string& error) {
        if (!Consume('"')) {
            error = "Expected string.";
            return false;
        }

        out.clear();
        while (!End()) {
            char ch = Advance();
            if (ch == '"') {
                return true;
            }
            if (ch == '\\') {
                if (End()) {
                    error = "Invalid escape sequence.";
                    return false;
                }
                char escaped = Advance();
                switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default:
                    error = "Unsupported escape sequence.";
                    return false;
                }
            } else {
                out.push_back(ch);
            }
        }

        error = "Unterminated string.";
        return false;
    }

    void SkipWhitespace() {
        while (!End() && std::isspace(static_cast<unsigned char>(m_text[m_cursor])) != 0) {
            ++m_cursor;
        }
    }

    bool Consume(char expected) {
        if (End() || m_text[m_cursor] != expected) {
            return false;
        }
        ++m_cursor;
        return true;
    }

    bool StartsWith(const char* text) const {
        size_t i = 0;
        while (text[i] != '\0') {
            if (m_cursor + i >= m_text.size() || m_text[m_cursor + i] != text[i]) {
                return false;
            }
            ++i;
        }
        return true;
    }

    char Peek() const {
        return End() ? '\0' : m_text[m_cursor];
    }

    char Advance() {
        return m_text[m_cursor++];
    }

    bool End() const {
        return m_cursor >= m_text.size();
    }

    std::string m_text;
    size_t m_cursor = 0;
};

} // namespace animpanel
