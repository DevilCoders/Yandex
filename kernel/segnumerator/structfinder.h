#pragma once

#include <util/generic/vector.h>

namespace NSegm {
namespace NPrivate {

struct TSegContext;
class TDocNode;

class TStructFinder {
private:
    TSegContext* Ctx = nullptr;
    TVector<TVector<TDocNode* > > Repeats;
    TVector<TDocNode* > ExplicitTables;
    TVector<TDocNode* > TableRows;
    TVector<TDocNode* > TableCells;
    TVector<TDocNode* > ExplicitLists;
    TVector<size_t> ListDepth;
private:
    void UpdateGroup();
    void Traverse(TDocNode* node);
    void FindCells(TDocNode* row);
    void FindRows(TDocNode* node);
    void FilterTables();
    void FillCtx();
    void MergeExplicitLists();
    void FilterRepeats();
    void DetectInnerLists();

public:
    TStructFinder(TSegContext* ctx);
    void FindAndFillCtx(TDocNode* node);
};

}
}
