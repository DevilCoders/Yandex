#include <kernel/urlid/doc_handle.h>

#include <util/stream/input.h>
#include <util/string/vector.h>
#include <util/string/split.h>

/**
 * Usage: ./doc_handle_decoder <<< 'route/id'
 * This tool behaves like c++filt: it fixes all docid-like strings in input,
 * leaving all other input as-is.
 */

int main(int, char*[]) {
    TString line;
    while (Cin.ReadLine(line)) {
        TVector<TString> tokens;
        for (auto& v : StringSplitter(line).SplitBySet(" \t\n,;=&")) {
            TString delim = TString(v.Delim());
            tokens.push_back(TString(v.Token()));
            if (!delim.empty()) {
                tokens.emplace_back(delim);
            }
        }
        for (TStringBuf token : tokens) {
            auto slash = token.find('/');
            if (slash == token.npos) {
                Cout << token;
                continue;
            }
            try {
                const auto route = FromString<TDocRoute::TRawType>(token.substr(0, slash));
                const auto hash = FromString<TDocHandle::THash>(token.substr(slash + 1));
                Cout << TDocHandle(hash, route);
            } catch (yexception&) {
                Cout << token;
            }
        }
        Cout << "\n";
    }
}
