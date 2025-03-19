#pragma once

#include <library/cpp/offroad/streams/bit_output.h>


namespace NDoom {

template<class CheckSumExtendCalcer>
class TCalcCheckSumStream : public IOutputStream {
public:
    using TCheckSum = typename CheckSumExtendCalcer::TCheckSum;

public:
    TCalcCheckSumStream()
        : CheckSum_(0)
    {}

    TCalcCheckSumStream(const TCalcCheckSumStream& stream)
        : CheckSum_(stream.CheckSum_)
        , ExtendCalcer_(stream.ExtendCalcer_)
    {}

    void Reset() {
        CheckSum_ = 0;
    }

    void WriteZeroCheckSum() {
        CheckSum_ = ExtendCalcer_(CheckSum_, CheckSumZeroBuffer_, sizeof(CheckSumZeroBufferHolder_));
    }

    void Flush(IOutputStream* target) {
        BitOutput_.Reset(target);
        BitOutput_.Write(CheckSum_, 8 * sizeof(CheckSum_));
        BitOutput_.Finish();
        Reset();
    }

    static void FlushZeroCheckSum(IOutputStream* target) {
        target->Write(CheckSumZeroBuffer_, sizeof(CheckSumZeroBufferHolder_));
    }

    size_t Size() const {
        return sizeof(CheckSum_);
    }

    TCheckSum CheckSum() const {
        return CheckSum_;
    }

protected:
    void DoWrite(const void* buf, size_t len) override {
        if (len > 0) {
            CheckSum_ = ExtendCalcer_(CheckSum_, buf, len);
        }
    }

private:
    TCheckSum CheckSum_;
    NOffroad::TBitOutput BitOutput_;
    CheckSumExtendCalcer ExtendCalcer_;

    static const TCheckSum CheckSumZeroBufferHolder_;
    static const void* CheckSumZeroBuffer_;
};

} // namespace NDoom

template<class T>
const typename NDoom::TCalcCheckSumStream<T>::TCheckSum NDoom::TCalcCheckSumStream<T>::CheckSumZeroBufferHolder_ = 0;

template<class T>
const void* NDoom::TCalcCheckSumStream<T>::CheckSumZeroBuffer_ = static_cast<const void*>(&NDoom::TCalcCheckSumStream<T>::CheckSumZeroBufferHolder_);
