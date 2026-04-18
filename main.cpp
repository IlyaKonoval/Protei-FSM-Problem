#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using std::string;
using std::string_view;

// ---------------------------------------------------------------------------
// Data types
// ---------------------------------------------------------------------------

struct FsmKey {
    string  name;
    int64_t id;

    bool operator==(const FsmKey& o) const noexcept {
        return id == o.id && name == o.name;
    }
};

struct FsmKeyHash {
    size_t operator()(const FsmKey& k) const noexcept {
        size_t h = std::hash<string>{}(k.name);
        size_t g = std::hash<int64_t>{}(k.id);
        return h ^ (g + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }
};

struct FsmState {
    string last_state_change_ts;
    string state;
    string last_event;
};

enum class LineKind { Input, Transition, Other };

struct Parsed {
    LineKind    kind  = LineKind::Other;
    string_view ts;
    string_view name;
    int64_t     id    = 0;
    string_view state;
    string_view event;
};

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

constexpr size_t kTsLen = 23; // "YYYY-MM-DD HH:MM:SS.mmm"

string_view trim(string_view s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\r')) --b;
    return s.substr(a, b - a);
}

bool has_ts_prefix(string_view line) {
    if (line.size() < kTsLen) return false;
    return line[4]  == '-' && line[7]  == '-' && line[10] == ' ' &&
           line[13] == ':' && line[16] == ':' && line[19] == '.';
}

bool parse_line(string_view line, Parsed& out) {
    if (!has_ts_prefix(line) || line.size() < kTsLen + 10) return false;
    out.ts = line.substr(0, kTsLen);

    size_t fsm_pos = line.find("FSM:", kTsLen);
    if (fsm_pos == string_view::npos) return false;

    size_t p = fsm_pos + 4;
    while (p < line.size() && line[p] == ' ') ++p;

    size_t id_kw = line.find(" id:", p);
    if (id_kw == string_view::npos) return false;
    out.name = trim(line.substr(p, id_kw - p));
    if (out.name.empty()) return false;

    p = id_kw + 4;
    while (p < line.size() && line[p] == ' ') ++p;
    size_t semi = line.find(';', p);
    if (semi == string_view::npos) return false;
    string_view id_sv = trim(line.substr(p, semi - p));
    int64_t id = 0;
    auto [ptr, ec] = std::from_chars(id_sv.data(), id_sv.data() + id_sv.size(), id);
    if (ec != std::errc()) return false;
    out.id = id;

    p = semi + 1;
    while (p < line.size() && line[p] == ' ') ++p;
    if (p >= line.size()) return false;
    char dir = line[p];
    if (dir != '>' && dir != '<') return false;
    ++p;
    while (p < line.size() && line[p] == ' ') ++p;

    if (p + 3 > line.size() || std::memcmp(line.data() + p, "St:", 3) != 0) return false;
    p += 3;
    while (p < line.size() && line[p] == ' ') ++p;

    while (p < line.size() && line[p] >= '0' && line[p] <= '9') ++p;
    while (p < line.size() && line[p] == ' ') ++p;

    size_t state_start = p;
    while (p < line.size() && line[p] != ' ' && line[p] != '\r' && line[p] != '\n') ++p;
    out.state = line.substr(state_start, p - state_start);
    if (out.state.empty()) return false;

    if (dir == '<') {
        out.kind = LineKind::Transition;
        return true;
    }

    out.kind = LineKind::Input;
    size_t pr_pos = line.find("Pr:", p);
    if (pr_pos == string_view::npos) {
        out.event = {};
        return true;
    }
    size_t q = pr_pos + 3;
    while (q < line.size() && line[q] == ' ') ++q;
    while (q < line.size() && line[q] != ' ' && line[q] != '\r' && line[q] != '\n') ++q;
    while (q < line.size() && line[q] == ' ') ++q;
    size_t ev_start = q;
    while (q < line.size() && line[q] != ' ' && line[q] != '(' &&
           line[q] != '\r'  && line[q] != '\n') ++q;
    out.event = line.substr(ev_start, q - ev_start);
    return true;
}

// ---------------------------------------------------------------------------
// End-state classifier
// ---------------------------------------------------------------------------

struct EndStates {
    std::unordered_map<string, std::unordered_set<string>> by_class;
    std::vector<string>                                     sorted_classes;
    mutable std::unordered_map<string, string>              pos_cache;
    mutable std::unordered_set<string>                      neg_cache;

    bool load(const string& path) {
        std::ifstream f(path);
        if (!f) {
            std::cerr << "error: cannot open end states file: " << path << "\n";
            return false;
        }
        string line;
        while (std::getline(f, line)) {
            string_view sv = trim(line);
            if (sv.empty() || sv[0] == '#') continue;
            size_t colon = sv.find(':');
            if (colon == string_view::npos) continue;
            string_view name = trim(sv.substr(0, colon));
            string_view st   = trim(sv.substr(colon + 1));
            if (name.empty() || st.empty()) continue;
            by_class[string(name)].insert(string(st));
        }
        sorted_classes.reserve(by_class.size());
        for (const auto& kv : by_class) sorted_classes.push_back(kv.first);
        std::sort(sorted_classes.begin(), sorted_classes.end(),
                  [](const string& a, const string& b) { return a.size() > b.size(); });
        return true;
    }

    const string& classify(const string& name) const {
        static const string kEmpty;
        auto pit = pos_cache.find(name);
        if (pit != pos_cache.end()) return pit->second;
        if (neg_cache.count(name)) return kEmpty;

        for (const string& cls : sorted_classes) {
            if (name == cls ||
                (name.size() > cls.size() &&
                 name.compare(0, cls.size(), cls) == 0 &&
                 name[cls.size()] == '.')) {
                auto [it, _] = pos_cache.emplace(name, cls);
                return it->second;
            }
        }
        neg_cache.insert(name);
        return kEmpty;
    }

    bool is_tracked(const string& name) const {
        return !classify(name).empty();
    }

    bool is_terminal(const string& name, const string& state) const {
        const string& cls = classify(name);
        if (cls.empty()) return false;
        auto it = by_class.find(cls);
        return it != by_class.end() && it->second.count(state) > 0;
    }
};

// ---------------------------------------------------------------------------
// Output helpers
// ---------------------------------------------------------------------------

string format_delta(const string& a, const string& b) {
    auto as_ms = [](const string& ts) -> int64_t {
        int H  = std::stoi(ts.substr(11, 2));
        int M  = std::stoi(ts.substr(14, 2));
        int S  = std::stoi(ts.substr(17, 2));
        int ms = std::stoi(ts.substr(20, 3));
        int y  = std::stoi(ts.substr(0, 4));
        int mo = std::stoi(ts.substr(5, 2));
        int d  = std::stoi(ts.substr(8, 2));
        std::tm tm{};
        tm.tm_year = y - 1900;
        tm.tm_mon  = mo - 1;
        tm.tm_mday = d;
#if defined(_WIN32)
        time_t day = _mkgmtime(&tm);
#else
        time_t day = timegm(&tm);
#endif
        return static_cast<int64_t>(day) * 1000 +
               ((H * 3600 + M * 60 + S) * 1000LL) + ms;
    };

    int64_t dms = as_ms(b) - as_ms(a);
    if (dms < 0) dms = 0;
    int64_t h   = dms / 3600000;
    int64_t rem = dms % 3600000;
    int64_t m   = rem / 60000;
    rem        %= 60000;
    int64_t s   = rem / 1000;
    int64_t ms  = rem % 1000;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld:%02lld.%03lld",
                  static_cast<long long>(h), static_cast<long long>(m),
                  static_cast<long long>(s), static_cast<long long>(ms));
    return string(buf);
}

string csv_field(const string& s) {
    if (s.find_first_of(",\"\r\n") == string::npos) return s;
    string r = "\"";
    for (char c : s) {
        if (c == '"') r += '"';
        r += c;
    }
    return r + '"';
}

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------

int64_t file_size(const string& path) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return -1;
    return static_cast<int64_t>(st.st_size);
}

void print_progress(const string& file, int64_t bytes, int64_t total, int pct) {
    std::fprintf(stderr, "\r[progress] %s: %d%% (%lld/%lld bytes)   ",
                 file.c_str(), pct,
                 static_cast<long long>(bytes),
                 static_cast<long long>(total));
    std::fflush(stderr);
}

void print_usage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " <end_states.txt> <out.csv> <log1> [<log2> ...]\n";
}

}  // namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    const string        end_states_path = argv[1];
    const string        out_path        = argv[2];
    std::vector<string> inputs;
    inputs.reserve(argc - 3);
    for (int i = 3; i < argc; ++i) inputs.emplace_back(argv[i]);

    EndStates end_states;
    if (!end_states.load(end_states_path)) return 2;

    std::unordered_map<FsmKey, FsmState, FsmKeyHash> fsms;
    fsms.reserve(1 << 16);

    string global_last_ts;

    for (const auto& in_path : inputs) {
        std::ifstream f(in_path);
        if (!f) {
            std::cerr << "error: cannot open input: " << in_path << "\n";
            return 3;
        }

        int64_t total    = file_size(in_path);
        int64_t bytes    = 0;
        int     last_pct = -1;
        string  line;
        line.reserve(512);

        while (std::getline(f, line)) {
            bytes += static_cast<int64_t>(line.size()) + 1;
            if (total > 0) {
                int pct = static_cast<int>((bytes * 100) / total);
                if (pct != last_pct) {
                    print_progress(in_path, bytes, total, pct);
                    last_pct = pct;
                }
            }

            if (has_ts_prefix(line)) {
                string_view ts = string_view(line).substr(0, kTsLen);
                if (ts > global_last_ts) global_last_ts = string(ts);
            }

            Parsed p;
            if (!parse_line(line, p)) continue;

            FsmKey key{string(p.name), p.id};
            if (!end_states.is_tracked(key.name)) continue;

            auto it = fsms.find(key);
            if (it == fsms.end()) {
                FsmState st;
                st.last_state_change_ts = string(p.ts);
                st.state                = string(p.state);
                if (p.kind == LineKind::Input) st.last_event = string(p.event);
                fsms.emplace(std::move(key), std::move(st));
                continue;
            }

            FsmState& st = it->second;
            if (p.kind == LineKind::Input) {
                st.last_event = string(p.event);
                st.state      = string(p.state);
            } else {
                st.last_state_change_ts = string(p.ts);
                st.state                = string(p.state);
            }
        }

        std::fprintf(stderr, "\r[progress] %s: done (%lld bytes)          \n",
                     in_path.c_str(), static_cast<long long>(bytes));
    }

    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "error: cannot open output: " << out_path << "\n";
        return 4;
    }

    std::vector<std::pair<FsmKey, FsmState>> stuck;
    for (auto& [key, st] : fsms) {
        if (!end_states.is_terminal(key.name, st.state))
            stuck.emplace_back(key, st);
    }
    std::sort(stuck.begin(), stuck.end(), [](const auto& a, const auto& b) {
        if (a.second.last_state_change_ts != b.second.last_state_change_ts)
            return a.second.last_state_change_ts < b.second.last_state_change_ts;
        if (a.first.name != b.first.name)
            return a.first.name < b.first.name;
        return a.first.id < b.first.id;
    });

    for (const auto& [key, st] : stuck) {
        string dur = global_last_ts.empty()
                         ? string("00:00:00.000")
                         : format_delta(st.last_state_change_ts, global_last_ts);
        out << csv_field(st.last_state_change_ts) << ','
            << csv_field(key.name)                << ','
            << key.id                             << ','
            << csv_field(st.state)                << ','
            << csv_field(st.last_event)           << ','
            << dur                                << '\n';
    }

    std::fprintf(stderr, "[done] %zu stuck FSM(s) written to %s\n",
                 stuck.size(), out_path.c_str());
    return 0;
}
