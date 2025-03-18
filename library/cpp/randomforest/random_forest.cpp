#include "random_forest.h"

#include <util/stream/file.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/generic/ymath.h>
#include <util/generic/yexception.h>
#include <util/string/split.h>

template <typename TChr>
inline size_t FastSplit(const TBasicStringBuf<TChr>& str, TChr sep, TVector<TBasicStringBuf<TChr>>& result) {
    result.clear();

    typedef TContainerConsumer<TVector<TBasicStringBuf<TChr>>> TConsumer;
    TConsumer consumer(&result);
    TSkipEmptyTokens<TConsumer> filter(&consumer);
    SplitString(str.data(), str.data() + str.size(), TCharDelimiter<const TChr>(sep), filter);

    return result.size();
}

inline TStringBuf NextLine(TStringBuf& str) {
    TStringBuf line = str.NextTok('\n');
    if (!line.empty() && line.back() == '\r')
        line.Chop(1);
    return line;
}

// TRandomTree

class TRandomTree {
public:
    virtual ~TRandomTree() {
    }

    virtual bool Predict(const TVector<unsigned>& factors, unsigned& res) const = 0;
};

class TMaxRandomTree;

template <typename TValue, typename TVarIndex, typename TNodeIndex>
class TRandomTreeImpl: public TRandomTree {
public:
    inline TRandomTreeImpl() {
    }

    // compacting copy constructor
    TRandomTreeImpl(const TMaxRandomTree& tree);

    bool Predict(const TVector<unsigned>& factors, unsigned& res) const override;

protected:
    struct TNode {
        TValue Value;

        typedef TNodeIndex TIndexes[2];
        TIndexes Indexes;

        TVarIndex BestVar;
        bool NodeStatus; // true if this is a leaf node (m_nodestatus == -1)

        inline TNode()
            : Value(0)
            , BestVar(0)
            , NodeStatus(false)
        {
            Indexes[0] = 0;
            Indexes[1] = 0;
        }
    };

    TVector<TNode> Nodes;

    template <typename T1, typename T2, typename T3>
    friend class TRandomTreeImpl;
};

using TContinuation = TVector<ui32>;

class TMaxRandomTree: public TRandomTreeImpl<ui64, ui32, ui32> {
public:
    bool ReadTree(size_t size, size_t offset, size_t structSize,
                  const TVector<unsigned>& nodepred,
                  const TVector<int>& nodestatus,
                  const TVector<unsigned>& treemap,
                  const TVector<unsigned>& xbestsplit,
                  const TVector<unsigned>& bestvar);

    TAutoPtr<TRandomTree> Compact() const;

    void Clear() {
        Nodes.clear();
    }

    // Reorder the nodes so that you get less cache misses
    // when jumping from root to leaves.
    // The algorithm is taken from Ivan Puzyrevsky's lecture at
    // Yandex Data Analysis School on 15/X 2012
    // (part of the course on external memory algorithms).
    void MakeVanEmdeBoasLayout();

private:
    size_t CalcDepth(ui32 root = 0) const;

    // Recursive call for the van Emde Boas layout.
    // Computes the new locations for the nodes
    // of the subtree of given depth with the given root.
    // Precondition:
    // newLocations.size() >= Nodes.size(),
    // newLocations[root] is already correct.
    // The nodes of the subtree are placed contiguously
    // after the root.
    // Return value: the first new location that is not occupied.
    // On return, cont has the immediate childern of the subtree
    // appended to it.
    ui32 ComputeLayoutForSubtree(ui32 root, size_t depth, TVector<ui32>& newLocations, TContinuation& cont) const;

    // Move the nodes to the new locations and update links.
    void RelocateNodes(const TVector<ui32>& newLocations);
};

template <typename TValue, typename TVarIndex, typename TNodeIndex>
bool TRandomTreeImpl<TValue, TVarIndex, TNodeIndex>::Predict(const TVector<unsigned>& factors, unsigned& res) const {
    //correct input size?

    TNodeIndex curNodeIndex = 0;
    while (curNodeIndex < Nodes.size()) {
        //reached the leaf?
        const TNode& curNode = Nodes[curNodeIndex];
        if (curNode.NodeStatus) {
            res = static_cast<unsigned>(curNode.Value);
            return true;
        }

        //check the condition
        size_t pos = 1 * ((int)(factors[curNode.BestVar] >= curNode.Value));
        curNodeIndex = curNode.Indexes[pos];

        //reached leaf with wrong status?
        if (curNodeIndex == 0)
            return false;
    }

    return false;
}

// filling is implemented for TMaxRandomTree only
bool TMaxRandomTree::ReadTree(size_t size, size_t offset, size_t structSize,
                              const TVector<unsigned>& nodepred,
                              const TVector<int>& nodestatus,
                              const TVector<unsigned>& treemap,
                              const TVector<unsigned>& xbestsplit,
                              const TVector<unsigned>& bestvar) {
    Clear();

    //check the sizes of input vectors
    size_t maxSize = offset + size;

    if (nodepred.size() < maxSize) {
        ythrow yexception() << "RandomForest - nodepred has wrong size";
        return false;
    }
    if (nodestatus.size() < maxSize) {
        ythrow yexception() << "RandomForest - nodestatus has wrong size";
        return false;
    }
    if (treemap.size() < 2 * maxSize) {
        ythrow yexception() << "RandomForest - treemap has wrong size";
        return false;
    }
    if (xbestsplit.size() < maxSize) {
        ythrow yexception() << "RandomForest - xbestsplit has wrong size";
        return false;
    }
    if (bestvar.size() < maxSize) {
        ythrow yexception() << "RandomForest - bestvar has wrong size";
        return false;
    }

    Nodes.resize(size);

    //populate Nodes with data from input vectors
    for (size_t i = 0; i != size; ++i) {
        TNode& node = Nodes[i];
        size_t ii = i + offset;

        node.NodeStatus = (nodestatus[ii] == -1);
        if (!node.NodeStatus && (nodepred[ii] != 0 || bestvar[ii] == 0)) {
            Clear();
            ythrow yexception() << "RandomForest - invalid data";
            return false;
        }

        //variables numbers begins from 0 instead of 1
        node.BestVar = bestvar[ii] ? bestvar[ii] - 1 : 0;
        if (node.NodeStatus) {
            node.Value = nodepred[ii];
        } else {
            node.Value = xbestsplit[ii];
        }

        unsigned left = treemap[offset * 2 + i];
        unsigned right = treemap[offset * 2 + i + structSize];

        //decrease by 1 to be zero-based numbering
        left = left ? (left - 1) : 0;
        right = right ? (right - 1) : 0;

        node.Indexes[0] = left;
        node.Indexes[1] = right;

        //if terminal leaf and everything's ok - continue
        if (!left && !right && node.Value > 0 && node.NodeStatus)
            continue;

        //nodes shouldn't point either outside of the tree or backwards (to avoid recursion)
        if (left >= size || right >= size || left <= i || right <= i) {
            Clear();
            return false;
        }
    }

    return true;
}

template <typename T>
static inline void SetMax(ui64& max, T cur) {
    if (max < cur)
        max = cur;
}

template <typename T>
static inline bool IsValid(ui64 v) {
    return v <= static_cast<ui64>(Max<T>());
}

template <typename TValue, typename TVarIndex>
static TRandomTree* CopyRandomTree(const TMaxRandomTree& source, ui64 maxNodeIndex) {
    // ui8 is unlikely to be a node index range, skip it to minimize a number of template specializations
    if (IsValid<ui16>(maxNodeIndex))
        return new TRandomTreeImpl<TValue, TVarIndex, ui16>(source);
    else
        return new TRandomTreeImpl<TValue, TVarIndex, ui32>(source);
}

template <typename TValue>
static TRandomTree* CopyRandomTree(const TMaxRandomTree& source, ui64 maxVarIndex, ui64 maxNodeIndex) {
    if (IsValid<ui8>(maxVarIndex))
        return CopyRandomTree<TValue, ui8>(source, maxNodeIndex);
    else if (IsValid<ui16>(maxVarIndex))
        return CopyRandomTree<TValue, ui16>(source, maxNodeIndex);
    else
        return CopyRandomTree<TValue, ui32>(source, maxNodeIndex);
}

static TRandomTree* CopyRandomTree(const TMaxRandomTree& source, ui64 maxValue, ui64 maxVarIndex, ui64 maxNodeIndex) {
    // ui8 is unlikely to be a values range, skip it
    if (IsValid<ui16>(maxValue))
        return CopyRandomTree<ui16>(source, maxVarIndex, maxNodeIndex);
    else if (IsValid<ui32>(maxValue))
        return CopyRandomTree<ui32>(source, maxVarIndex, maxNodeIndex);
    else
        return CopyRandomTree<ui64>(source, maxVarIndex, maxNodeIndex);
}

size_t TMaxRandomTree::CalcDepth(ui32 root) const {
    const TNode& node = Nodes[root];
    if (node.NodeStatus) {
        return 0;
    }
    return 1 + Max(CalcDepth(node.Indexes[0]), CalcDepth(node.Indexes[1]));
}

ui32 TMaxRandomTree::ComputeLayoutForSubtree(ui32 root, size_t depth,
                                             TVector<ui32>& newLocations,
                                             TContinuation& result) const {
    if (depth == 0) {
        const TNode& node = Nodes[root];
        if (!node.NodeStatus) {
            result.push_back(node.Indexes[0]);
            result.push_back(node.Indexes[1]);
        }
        return newLocations[root] + 1;
    }
    const size_t topDepth = (depth - 1) / 2;
    const size_t bottomDepth = depth - 1 - topDepth;
    TContinuation section;
    ui32 end = ComputeLayoutForSubtree(root, topDepth, newLocations, section);
    for (auto newRoot : section) {
        newLocations[newRoot] = end;
        end = ComputeLayoutForSubtree(newRoot, bottomDepth, newLocations, result);
    }
    return end;
}

void TMaxRandomTree::RelocateNodes(const TVector<ui32>& newLocations) {
    TVector<TNode> newNodes(Nodes.size());
    for (size_t i = 0; i < Nodes.size(); ++i) {
        TNode& newNode = newNodes[newLocations[i]];
        newNode = Nodes[i];
        newNode.Indexes[0] = newLocations[newNode.Indexes[0]];
        newNode.Indexes[1] = newLocations[newNode.Indexes[1]];
    }
    newNodes.swap(Nodes);
}

void TMaxRandomTree::MakeVanEmdeBoasLayout() {
    TVector<ui32> newLocations(Nodes.size(), 0);
    const size_t depth = CalcDepth();
    TContinuation dummy;
    ComputeLayoutForSubtree(0, depth, newLocations, dummy);
    RelocateNodes(newLocations);
}

TAutoPtr<TRandomTree> TMaxRandomTree::Compact() const {
    ui64 maxValue = 0;
    ui64 maxVarIndex = 0;
    ui64 maxNodeIndex = 0;

    for (size_t i = 0; i < Nodes.size(); ++i) {
        const TNode& node = Nodes[i];
        SetMax(maxValue, node.Value);
        SetMax(maxVarIndex, node.BestVar);
        SetMax(maxNodeIndex, Max(node.Indexes[0], node.Indexes[1]));
    }
    return CopyRandomTree(*this, maxValue, maxVarIndex, maxNodeIndex);
}

// compacting copy constructor
template <typename TValue, typename TVarIndex, typename TNodeIndex>
TRandomTreeImpl<TValue, TVarIndex, TNodeIndex>::TRandomTreeImpl(const TMaxRandomTree& tree) {
    Nodes.resize(tree.Nodes.size());
    for (size_t i = 0; i < tree.Nodes.size(); ++i) {
        Nodes[i].Value = static_cast<TValue>(tree.Nodes[i].Value);

        Nodes[i].Indexes[0] = static_cast<TNodeIndex>(tree.Nodes[i].Indexes[0]);
        Nodes[i].Indexes[1] = static_cast<TNodeIndex>(tree.Nodes[i].Indexes[1]);

        Nodes[i].BestVar = static_cast<TVarIndex>(tree.Nodes[i].BestVar);
        Nodes[i].NodeStatus = tree.Nodes[i].NodeStatus;
    }
}

// TRandomForest

TRandomForest::TRandomForest()
    : Invfs(0.0f)
{
}

TRandomForest::~TRandomForest() {
}

template <typename T>
static void ReadNumbersFromFile(const TString& fileName, TVector<T>& numbers) {
    TBlob file = TBlob::FromFileContent(fileName);
    ReadNumbersFromFile(file, numbers);
}

template <typename T>
static void ReadNumbersFromFile(const TBlob& file, TVector<T>& numbers) {
    TStringBuf data(file.AsCharPtr(), file.Size());
    TVector<TStringBuf> fields;
    while (!data.empty()) {
        TStringBuf line = NextLine(data);
        FastSplit(line, ' ', fields);
        for (size_t i = 0; i < fields.size(); ++i)
            numbers.push_back(::FromString<T>(fields[i]));
    }
}

bool TRandomForest::ReadRF(const TString& prefix, size_t trees) {
    TBlob blobs[] = {TBlob::FromFileContent(prefix + "ndbigtree"), TBlob::FromFileContent(prefix + "bestvar"),
                     TBlob::FromFileContent(prefix + "nodepred"), TBlob::FromFileContent(prefix + "nodestatus"),
                     TBlob::FromFileContent(prefix + "treemap"), TBlob::FromFileContent(prefix + "xbestsplit")};
    return ReadRF(blobs, trees);
}

bool TRandomForest::ReadRF(TBlob blobs[6], size_t trees) {
    Forest.clear();
    TVector<ui32> ndBigTree;

    size_t maxTreeSize = 0;

    TVector<TStringBuf> fields;

    TBlob& ndbigtreeFile(blobs[0]);
    TStringBuf ndbigtreeData(ndbigtreeFile.AsCharPtr(), ndbigtreeFile.Size());
    while (!ndbigtreeData.empty()) {
        TStringBuf line = NextLine(ndbigtreeData);
        FastSplit(line, ' ', fields);
        for (size_t i = 0; i != fields.size(); ++i) {
            size_t treeSize = ::FromString<size_t>(fields[i]);
            ndBigTree.push_back(treeSize);
            maxTreeSize = Max(treeSize, maxTreeSize);
        }
    }

    size_t structSize = maxTreeSize * ndBigTree.size();

    TVector<unsigned> bestvar;
    ReadNumbersFromFile(blobs[1], bestvar);
    if (bestvar.size() != structSize)
        return false;

    TVector<unsigned> nodepred;
    ReadNumbersFromFile(blobs[2], nodepred);
    if (nodepred.size() != structSize)
        return false;

    TVector<int> nodestatus;
    ReadNumbersFromFile(blobs[3], nodestatus);
    if (nodestatus.size() != structSize)
        return false;

    TVector<unsigned> treemap;
    ReadNumbersFromFile(blobs[4], treemap);
    if (treemap.size() != structSize * 2)
        return false;

    TBlob& xbestsplitFile(blobs[5]);
    TStringBuf xbestsplitData(xbestsplitFile.AsCharPtr(), xbestsplitFile.Size());
    TVector<unsigned> xbestsplit;
    while (!xbestsplitData.empty()) {
        TStringBuf line = NextLine(xbestsplitData);
        FastSplit(line, ' ', fields);
        for (size_t i = 0; i != fields.size(); ++i)
            xbestsplit.push_back(unsigned(ceil(::FromString<double>(fields[i])) + 0.1));
    }
    if (xbestsplit.size() != structSize) {
        ythrow yexception() << "wrong xbestsplit.size";
        return false;
    }

    if (trees && ndBigTree.size() > trees)
        ndBigTree.resize(trees);

    size_t offset = 0;
    TMaxRandomTree tree;
    for (size_t i = 0; i < ndBigTree.size(); ++i) {
        if (!tree.ReadTree(ndBigTree[i], offset, maxTreeSize, nodepred, nodestatus, treemap, xbestsplit, bestvar)) {
            Forest.clear();
            return false;
        }
        tree.MakeVanEmdeBoasLayout();
        Forest.push_back(tree.Compact());
        offset += maxTreeSize;
    }
    if (!Forest.empty())
        Invfs = 1.0f / Forest.size();
    else
        return false;
    return true;
}

const unsigned MAX_MODALITIES = 256;

bool TRandomForest::Predict(const TVector<unsigned>& factors, TVector<float>& res) const {
    res.clear();
    const size_t trees = Forest.size();
    for (size_t i = 0; i != trees; ++i) {
        unsigned r = 0;
        if (!Forest[i]->Predict(factors, r)) {
            ythrow yexception() << "can't predict with tree " << i;
            return false;
        } else {
            //result value is too big
            if (r >= MAX_MODALITIES)
                return false;

            if (res.size() <= r)
                res.resize(r + 1, 0.0);

            res[r] += Invfs;
        }
    }

    return true;
}
