#include "syntax_chunk.h"

#include <util/generic/utility.h>
#include <util/generic/ymath.h>

namespace NReMorph {

TSyntaxChunk::TSyntaxChunk()
    : Pos(0, 0)
    , Weight(0.0)
    , Depth(-1)
    , Head(-1)
{
}

TSyntaxChunk::TSyntaxChunk(size_t start, size_t end)
    : Pos(start, end)
    , Weight(0.0)
    , Depth(-1)
    , Head(-1)
{
}

TSyntaxChunk::TSyntaxChunk(const std::pair<size_t, size_t>& pos, float weight, const TString& name)
    : Pos(pos)
    , Weight(weight)
    , Name(name)
    , Depth(-1)
    , Head(-1)
{
}

// Calc and return current chunk depth. Bottom chunks have level 1
int TSyntaxChunk::GetDepth() {
    if (-1 == Depth) {
        int maxDepth = 0;
        for (size_t i = 0; i < Children.size(); ++i) {
            maxDepth = ::Max(maxDepth, Children[i].GetDepth());
        }
        Depth = maxDepth + 1;
    }
    return Depth;
}

// Add child chunk in proper level. The position of added chunk must belong to the position of the current one
bool TSyntaxChunk::AddChild(const TSyntaxChunk& newChunk) {
    Y_ASSERT(Pos != newChunk.Pos);
    Y_ASSERT(Include(newChunk.Pos));
    if (Children.empty()) {
        Children.push_back(newChunk);
        return true;
    } else {
        return AddChunk(Children, newChunk);
    }
}

void TSyntaxChunk::Swap(TSyntaxChunk& c) {
    ::DoSwap(Pos, c.Pos);
    ::DoSwap(Weight, c.Weight);
    ::DoSwap(Name, c.Name);
    ::DoSwap(Children, c.Children);
    ::DoSwap(Depth, c.Depth);
    ::DoSwap(Head, c.Head);
}

bool TSyntaxChunk::AddChunk(TVector<TSyntaxChunk>& chunks, const TSyntaxChunk& newChunk) {
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (chunks[i].Pos.second > newChunk.Pos.first) {
            if (chunks[i].Pos == newChunk.Pos) {
                if (-1 == chunks[i].Head)
                    chunks[i].Head = newChunk.Head;
                // |--chunk[i]--|
                // |--newChunk--|
                return newChunk.Name == chunks[i].Name;
            } else if (chunks[i].Conflict(newChunk.Pos)) {
                //    |--chunk[i]--|
                // |--newChunk--|
                // or
                // |--chunk[i]--|
                //    |--newChunk--|
                return false;
            } else if (chunks[i].Include(newChunk.Pos)) {
                // |----chunk[i]----|
                //   |--newChunk--|
                return chunks[i].AddChild(newChunk);
            } else if (newChunk.Include(chunks[i].Pos)) {
                // |----------------newChunk--------------------|
                //   |chunk[i]| |chunk[i+1]| ... |chunk[i+cnt]|
                // Replace current and all subsequent included chunks by the new one
                size_t cnt = 1;
                for (; i + cnt < chunks.size(); ++cnt) {
                    if (newChunk.Conflict(chunks[i + cnt].Pos))
                        return false;
                    if (!newChunk.Include(chunks[i + cnt].Pos))
                        break;
                }
                chunks.insert(chunks.begin() + i, newChunk);
                chunks[i].Children.assign(chunks.begin() + i + 1, chunks.begin() + i + cnt + 1);
                chunks.erase(chunks.begin() + i + 1, chunks.begin() + i + cnt + 1);
                return true;
            } else {
                //                |--chunk[i]--|
                // |--newChunk--|
                // Insert chunk before the current one
                chunks.insert(chunks.begin() + i, newChunk);
                return true;
            }
        }
    }
    // |--chunk[last]--|
    //                   |--newChunk--|
    chunks.push_back(newChunk);
    return true;
}

void TSyntaxChunk::SetHead(const std::pair<size_t, size_t>& headRange) {
    if (Include(headRange)) {
        Head = headRange.first;
        SetHead(Children, headRange);
    }
}

bool TSyntaxChunk::SetHead(const std::pair<size_t, size_t>& headRange, const std::pair<size_t, size_t>& parentRange) {
    if (Include(parentRange, Pos)) {
        if (!Include(headRange)) {
            return false;
        }
        Head = headRange.first;
        SetHead(Children, headRange, parentRange);
    } else if (Include(parentRange)) {
        if (!SetHead(Children, headRange, parentRange))
            Head = headRange.first;
    }
    return true;
}

void TSyntaxChunk::SetHead(TVector<TSyntaxChunk>& chunks, const std::pair<size_t, size_t>& headRange) {
    for (size_t i = 0; i < chunks.size(); ++i) {
        chunks[i].SetHead(headRange);
    }
}

bool TSyntaxChunk::SetHead(TVector<TSyntaxChunk>& chunks, const std::pair<size_t, size_t>& headRange, const std::pair<size_t, size_t>& parentRange) {
    bool res = false;
    for (size_t i = 0; i < chunks.size(); ++i) {
        res = chunks[i].SetHead(headRange, parentRange) || res;
    }
    return res;
}


// Calculate total coverage
size_t TSyntaxChunk::Coverage(const TVector<TSyntaxChunk>& chunks) {
    size_t res = 0;
    for (size_t i = 0; i < chunks.size(); ++i) {
        res += chunks[i].Length();
    }
    return res;
}

struct TPhraseTypeCounter {
    size_t XP;
    size_t QP;
    size_t NP;
    size_t VP;

    TPhraseTypeCounter()
        : XP(0)
        , QP(0)
        , NP(0)
        , VP(0)
    {
    }

    bool operator () (const TSyntaxChunk& chunk) {
        ++XP;
        if (chunk.Name.StartsWith("NP")) {
            ++NP;
        } else if (chunk.Name.StartsWith("VP")) {
            ++VP;
        } else if (chunk.Name.StartsWith("QP")) {
            ++QP;
        }
        return true;
    }
};

double TSyntaxChunk::Quality(size_t wordCount, TVector<TSyntaxChunk>& chunks) {
    TPhraseTypeCounter counts;
    Iter(chunks, counts);

    int depth = 0;
    for (size_t i = 0; i < chunks.size(); ++i) {
        depth = Max(depth, chunks[i].GetDepth());
    }

    const size_t coverage = Coverage(chunks);
    const int islands = wordCount - coverage + chunks.size();
    Y_ASSERT(islands > 0);

    const double z = (-1.0 * 3)
        + (0.01 * depth)
        + (2.0 * coverage)
        + (1.0 * (counts.XP ? 1 : 0))
        + (3.0 * (counts.NP ? 1 : 0))
        + (2.0 * (counts.VP ? counts.VP : 0))
        + (3.0 * (counts.QP ? counts.QP : 0))
        + (-5.0 * (islands == 1 ? 0 : islands));
    return 1.0 / (1.0 + std::exp(-z));
}

} // NReMorph
