#include "geotree.h"
#include <util/stream/file.h>
#include <util/generic/string.h>
#include <util/string/vector.h>
#include <util/string/split.h>

void TGeoTree::AddEdge(size_t parent, size_t child) {
    if (Data.size() <= child) {
        Data.resize(child + 1, 0);
    } else if (Data[child]) {
        throw TMalformedException() << "ancestor redefinition";
    }

    if (IsDescendant(child, parent)) {
        throw TMalformedException() << "closed loop";
    }

    Data[child] = parent;
}

void TGeoTree::Load(const char* path) {
    if (path && *path) {
        TFileInput instream(path);
        Load(instream);
    }
}

void TGeoTree::Load(IInputStream& in) {
    TString line;
    TVector<TString> tokens;
    size_t linecount = 0;

    while (in.ReadLine(line)) {
        linecount += 1;
        if (line.StartsWith('#'))
            continue;
        StringSplitter(line).SplitBySet(" \t\n\r").SkipEmpty().Collect(&tokens);
        if (tokens.size() != 2) {
            Cerr << "Malformed geo tree data file: bad token count at line " << linecount << Endl;
            continue;
        }
        size_t child = FromString<size_t>(tokens[0]);
        size_t parent = FromString<size_t>(tokens[1]);

        try {
            AddEdge(parent, child);
        } catch (const TMalformedException& e) {
            Cerr << "Malformed geo tree data file: " << e.what() << " at line " << linecount << Endl;
        }
    }
}
