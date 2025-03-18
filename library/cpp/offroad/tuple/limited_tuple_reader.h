#pragma once

#include "tuple_reader.h"

#include <utility>

#include <library/cpp/offroad/offset/data_offset.h>

namespace NOffroad {
    template <class Data, class Vectorizer, class Subtractor, class BaseDecoder = TDecoder64, size_t blockMultiplier = 1, EBufferType bufferType = AutoEofBuffer>
    class TLimitedTupleReader: public TTupleReader<Data, Vectorizer, Subtractor, BaseDecoder, blockMultiplier, bufferType> {
        using TBase = TTupleReader<Data, Vectorizer, Subtractor, BaseDecoder, blockMultiplier, bufferType>;

        enum {
            PrefixSize = Subtractor::PrefixSize == -1 ? static_cast<int>(TBase::TupleSize) : static_cast<int>(Subtractor::PrefixSize),
        };

    public:
        using THit = Data;

        template <class... Args>
        TLimitedTupleReader(Args&&... args)
            : TBase(std::forward<Args>(args)...)
        {
            ClearLimits();
        }

        TLimitedTupleReader(TLimitedTupleReader&& other)
            : TBase(std::move(other))
        {
            DoSwap(StartLimit_, other.StartLimit_);
            DoSwap(EndLimit_, other.EndLimit_);
        }

        template <class... Args>
        void Reset(Args&&... args) {
            TBase::Reset(std::forward<Args>(args)...);
            ClearLimits();
        }

        Y_FORCE_INLINE bool ReadHit(THit* data) {
            if (TBase::Position() >= EndLimit())
                return false;

            return TBase::ReadHit(data);
        }

        template <class Consumer>
        Y_FORCE_INLINE bool ReadHits(const Consumer& consumer) {
            TDataOffset position = TBase::Position();
            if (position.Offset() < EndLimit_.Offset()) {
                return TBase::ReadHits(consumer);
            }
            if (position < EndLimit_) {
                size_t left = EndLimit_.Index() - position.Index();
                auto limitedConsumer = [&](const THit& data) {
                    if (left > 0) {
                        --left;
                        return consumer(data);
                    } else {
                        return false;
                    }
                };

                return TBase::ReadHits(limitedConsumer);
            }
            return false;
        }

        template <class SeekMode = TIntegratingSeek>
        Y_FORCE_INLINE bool Seek(TDataOffset position, const THit& startData, SeekMode mode = SeekMode()) {
            if (std::is_same<SeekMode, TSeekPointSeek>::value && PrefixSize > 0) {
                return TBase::SeekInternal(position, startData, mode, position.Index(), true);
            } else {
                return TBase::Seek(position, startData, mode);
            }
        }

        Y_FORCE_INLINE bool LowerBoundLocal(const THit& prefix, THit* first) {
            TDataOffset position = TBase::Position();
            Y_ASSERT(position >= StartLimit_);
            if (position >= EndLimit_) {
                return false;
            }
            if (StartLimit_.Offset() < position.Offset() && position.Offset() < EndLimit_.Offset()) {
                return TBase::LowerBoundLocal(prefix, first);
            }
            const size_t from = (position.Offset() == StartLimit_.Offset() ? StartLimit_.Index() : 0);
            const size_t to = (position.Offset() == EndLimit_.Offset() ? EndLimit_.Index() : static_cast<size_t>(TBase::BlockSize));
            return TBase::Buffer_.LowerBound(prefix, first, from, to, position.Offset() == StartLimit_.Offset());
        }

        TDataOffset StartLimit() const {
            return StartLimit_;
        }

        TDataOffset EndLimit() const {
            return EndLimit_;
        }

        void SetLimits(const TDataOffset& startLimit, const TDataOffset& endLimit) {
            StartLimit_ = startLimit;
            EndLimit_ = endLimit;
        }

    private:
        void ClearLimits() {
            StartLimit_ = TDataOffset();
            EndLimit_ = TDataOffset::Max();
        }

    private:
        TDataOffset StartLimit_;
        TDataOffset EndLimit_;
    };

}
