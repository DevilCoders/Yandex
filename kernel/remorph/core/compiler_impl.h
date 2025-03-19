#pragma once

#include "nfa.h"
#include "types.h"
#include "ast.h"
#include "debug.h"

#include <util/generic/yexception.h>

#define DBGO GetDebugOutCOMPILER()

namespace NRemorph {

class TCompilerError: public yexception {
};

namespace NPrivate {

class TTooManyTags: public TCompilerError {
public:
    TTooManyTags() {
        *this << "Too many capturing parens";
    }
};

template <class TLiteralTable>
struct TSubmatchIdVisitor {
    const TLiteralTable& LiteralTable;
    TTagId& Next;
    TSubmatchIdToName& IdToName;
    TSubmatchIdToSourcePos& IdToSourcePos;
    TSubmatchIdVisitor(const TLiteralTable& lt, TTagId& next, TSubmatchIdToName& idToName, TSubmatchIdToSourcePos& idToSourcePos)
        : LiteralTable(lt)
        , Next(next)
        , IdToName(idToName)
        , IdToSourcePos(idToSourcePos) {
    }
    bool VisitBeforeChildren(TAstNode* node_) {
        static TTagId maxSubmatchId = Max<TTagId>() / 2;
        NWRED(DBGO << ToString(LiteralTable, *node_->Data) << Endl);
        if (TAstData::Submatch == TAstData::GetType(node_)) {
            TAstSubmatch* node = static_cast<TAstSubmatch*>(node_->Data.Get());
            node->Id = Next;
            if (!node->Name.empty())
                IdToName[Next] = node->Name;
            IdToSourcePos[Next] = node->SourcePos;
            if (Next == maxSubmatchId) {
                ythrow TTooManyTags();
            }
            ++Next;
        }
        return true;
    }
    bool VisitAfterChildren(TAstNode*) {
        return true;
    }
};

template <class TLiteralTable>
inline size_t CalcSubmatchIds(const TLiteralTable& lt, TAstNode* node, TSubmatchIdToName& idToName, TSubmatchIdToSourcePos& idToSourcePos) {
    TTagId id = 0;
    TSubmatchIdVisitor<TLiteralTable> v(lt, id, idToName, idToSourcePos);
    TAstTree::Traverse(node, v);
    return id;
}

void Compile(const TNFAPtr& nfa, TAstNode* ast);

} // NPrivate

} // NRemorph

#undef DBGO
