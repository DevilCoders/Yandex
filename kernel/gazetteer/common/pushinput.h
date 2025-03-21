#pragma once

#include <util/memory/blob.h>
#include <util/generic/stack.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>

#include <util/stream/mem.h>



class TPushInput: public IInputStream {
    struct TBlobInput: public TBlob, public TMemoryInput {
        TBlobInput(const TBlob& data)
            : TBlob(data)
            , TMemoryInput(TBlob::Data(), TBlob::Size())
        {
        }
    };

    public:
        TPushInput(IInputStream* input)
            : Input_(input)
        {
        }

        ~TPushInput() override {
        }

        void Push(const TBlob& data) {
            if (data.Size()) {
                Backups_.push(TBlobInput(data));
            }
        }

        void Push(const TString& data) {
            Push(TBlob::FromString(data));
        }

    private:
        size_t DoRead(void* buf, size_t len) override {
            if (Backups_.empty()) {
                return Input_->Read(buf, len);
            }

            size_t ret = Backups_.top().Read(buf, len);
            if (Backups_.top().Exhausted())
                Backups_.pop();
            return ret;
        }

    private:
        IInputStream* Input_;
        TStack<TBlobInput> Backups_;
};


class TSkipInput: public TPushInput {
public:
    TSkipInput(IInputStream* input)
        : TPushInput(input)
    {
    }

    bool Skip(const TString& header) {
        TString buffer;
        buffer.ReserveAndResize(header.size());
        buffer.resize(Load(buffer.begin(), buffer.size()));
        if (buffer == header)
            return true;    // skipped

        Push(buffer);
        return false;
    }
};

class TSkipBOMInput: public TSkipInput {
public:
    TSkipBOMInput(IInputStream* input)
        : TSkipInput(input)
        , Skipped(Skip("\xEF\xBB\xBF"))
    {
    }

    bool HasBOM() const {
        return Skipped;
    }

private:
    bool Skipped;
};

// capturing stream adaptor: takes ownership on given TFrom and transforms it to TTo
template <class TFrom, class TTo>
class TStreamAdaptor: private THolder<TFrom>, public TTo {
public:
    inline TStreamAdaptor(TFrom* slave)
        : THolder<TFrom>(slave)
        , TTo(slave)
    {
    }

    ~TStreamAdaptor() override {
    }
};

template <typename TResult, typename TInput>
TAutoPtr<IInputStream> MakeInputAdaptor(TInput* input) {
    return new TStreamAdaptor<TInput, TResult>(input);
}


