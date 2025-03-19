#pragma once

#include <util/generic/ptr.h>

template<typename TValueType>
class TAnyIterator {
protected:
    using TSelf = TAnyIterator<TValueType>;

    class TIteratorAdapterBase {
    public:
        virtual ~TIteratorAdapterBase() {}

        virtual THolder<TIteratorAdapterBase> Clone() const = 0;

        virtual TValueType& Get() const = 0;

        virtual void Advance() = 0;

        virtual bool Equals(TIteratorAdapterBase* rhs) const = 0;
    };

    template<typename TIterator>
    class TIteratorAdapter : public TIteratorAdapterBase {
    private:
        TIterator Iterator;

    public:
        TIteratorAdapter(const TIterator& iterator)
            : Iterator(iterator)
        {
        }

        THolder<TIteratorAdapterBase> Clone() const override {
            return THolder<TIteratorAdapterBase>(new TIteratorAdapter(Iterator));
        }

        TValueType& Get() const override {
            return *Iterator;
        }

        void Advance() override {
            ++Iterator;
        }

        bool Equals(TIteratorAdapterBase* rhs) const override {
            TIteratorAdapter<TIterator>* tmp = dynamic_cast<TIteratorAdapter<TIterator>*>(rhs);
            return tmp && Iterator == tmp->Iterator;
        }

    };

protected:
    THolder<TIteratorAdapterBase> Adapter;

public:
    TAnyIterator(const TSelf& iterator)
        : Adapter(iterator.Adapter ? iterator.Adapter->Clone() : nullptr)
    {
    }

    template<typename TIterator>
    TAnyIterator(const TIterator& iterator)
        : Adapter(new TIteratorAdapter<TIterator>(iterator))
    {
    }

    TSelf& operator=(const TSelf& rhs) {
        Adapter = rhs.Adapter->Clone();
        return *this;
    }

    TValueType& operator*() const {
        return Adapter->Get();
    }

    TValueType* operator->() const {
        return &(operator*());
    }

    TSelf& operator++() {
        Adapter->Advance();

        return *this;
    }

    TSelf operator++(int) {
        TSelf tmp(*this);

        Adapter->Advance();

        return tmp;
    }

    bool operator==(const TSelf& rhs) const {
        return Adapter->Equals(rhs.Adapter.Get());
    }

    bool operator!=(const TSelf& rhs) const {
        return !operator==(rhs);
    }
};

template<typename TValueType>
using TAnyConstIterator = TAnyIterator<const TValueType>;
