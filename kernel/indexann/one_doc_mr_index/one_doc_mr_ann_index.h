#pragma once

#include "one_doc_mr_ann_iterator.h"

#include <kernel/indexann/interface/reader.h>
#include <kernel/indexann/protos/portion.pb.h>

namespace NIndexAnn {
    class TOneDocMrIndex: public IDocDataIndex {
    public:
        TOneDocMrIndex(const T4DArrayRow* row)
            : Row_(row)
        {
        }

        virtual bool HasDoc(ui32 doc) const override {
            return doc == 0;
        }

    protected:
        THolder<IDocDataIterator> DoCreateIterator() const override {
            return MakeHolder<TOneDocMrIterator>(Row_);
        }

    private:
        const NIndexAnn::T4DArrayRow* Row_ = nullptr;
    };
} // namespace NIndexAnn
