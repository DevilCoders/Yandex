#include "block.h"

#include <util/generic/yexception.h>

namespace NClickHouse {
    TBlock::TIterator::TIterator(const TBlock& block)
        : Block_(block)
        , Idx_(0)
    {
    }

    const TString& TBlock::TIterator::Name() const {
        return Block_.Columns_[Idx_].Name;
    }

    TTypeRef TBlock::TIterator::Type() const {
        return Block_.Columns_[Idx_].Column->Type();
    }

    TColumnRef TBlock::TIterator::Column() const {
        return Block_.Columns_[Idx_].Column;
    }

    void TBlock::TIterator::Next() {
        ++Idx_;
    }

    bool TBlock::TIterator::IsValid() const {
        return Idx_ < Block_.Columns_.size();
    }

    TBlock::TBlock()
        : Rows_(0)
    {
    }

    TBlock::TBlock(size_t cols, size_t rows)
        : Rows_(rows)
    {
        Columns_.reserve(cols);
    }

    TBlock::~TBlock() = default;

    void TBlock::AppendColumn(const TString& name, const TColumnRef& col) {
        if (Columns_.empty()) {
            Rows_ = col->Size();
        } else if (col->Size() != Rows_) {
            ythrow yexception()
                << "all clumns in block must have same count of rows";
        }

        Columns_.push_back(TColumnItem{name, col});
    }

    /// Count of columns in the block.
    size_t TBlock::GetColumnCount() const {
        return Columns_.size();
    }

    const TBlockInfo& TBlock::Info() const {
        return Info_;
    }

    /// Count of rows in the block.
    size_t TBlock::GetRowCount() const {
        return Rows_;
    }

    TColumnRef TBlock::operator[](size_t idx) const {
        if (idx < Columns_.size()) {
            return Columns_[idx].Column;
        }

        ythrow yexception() << "column index is out of range";
    }

}
