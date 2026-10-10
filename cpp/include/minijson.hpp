#pragma once
// Tiny JSON reader for the shapes this project needs: an object, or an array of FLAT objects
// whose values are strings, numbers or booleans. Values are kept as text. No external library needed.
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace minijson {

using Object = std::map<std::string, std::string>;

class Parser {
public:
    explicit Parser(const std::string& s) : s_(s) {}

    // Accepts either one object or an array of objects.
    bool parse(std::vector<Object>& out) {
        ws();
        if (peek() == '{') { Object o; if (!object(o)) return false; out.push_back(std::move(o)); return true; }
        if (peek() != '[') return false;
        ++i_; ws();
        if (peek() == ']') { ++i_; return true; }
        while (true) {
            ws();
            Object o;
            if (!object(o)) return false;
            out.push_back(std::move(o));
            ws();
            if (peek() == ',') { ++i_; continue; }
            if (peek() == ']') { ++i_; return true; }
            return false;
        }
    }

private:
    char peek() const { return i_ < s_.size() ? s_[i_] : '\0'; }
    void ws() { while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_; }

    bool string(std::string& out) {
        if (peek() != '"') return false;
        ++i_; out.clear();
        while (i_ < s_.size() && s_[i_] != '"') {
            char c = s_[i_++];
            if (c == '\\' && i_ < s_.size()) {
                const char e = s_[i_++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'u': out += '?'; i_ = std::min(i_ + 4, s_.size()); break;   // non-ASCII not needed here
                    default: out += e;
                }
            } else out += c;
        }
        if (peek() != '"') return false;
        ++i_;
        return true;
    }
    bool scalar(std::string& out) {                                   // number, true, false, null
        const size_t b = i_;
        while (i_ < s_.size() && s_[i_] != ',' && s_[i_] != '}' && s_[i_] != ']' && !std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
        out = s_.substr(b, i_ - b);
        return !out.empty();
    }
    bool skipArray(std::string& out) {                                // arrays inside objects (e.g. "reasons") are kept as raw text
        const size_t b = i_; int depth = 0; bool inStr = false;
        for (; i_ < s_.size(); ++i_) {
            const char c = s_[i_];
            if (inStr) { if (c == '\\') ++i_; else if (c == '"') inStr = false; continue; }
            if (c == '"') inStr = true;
            else if (c == '[') ++depth;
            else if (c == ']' && --depth == 0) { ++i_; out = s_.substr(b, i_ - b); return true; }
        }
        return false;
    }
    bool object(Object& o) {
        if (peek() != '{') return false;
        ++i_; ws();
        if (peek() == '}') { ++i_; return true; }
        while (true) {
            ws();
            std::string k, v;
            if (!string(k)) return false;
            ws();
            if (peek() != ':') return false;
            ++i_; ws();
            if (peek() == '"') { if (!string(v)) return false; }
            else if (peek() == '[') { if (!skipArray(v)) return false; }
            else if (!scalar(v)) return false;
            o[k] = v;
            ws();
            if (peek() == ',') { ++i_; continue; }
            if (peek() == '}') { ++i_; return true; }
            return false;
        }
    }
    const std::string& s_;
    size_t i_ = 0;
};

inline std::string escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

}  // namespace minijson
