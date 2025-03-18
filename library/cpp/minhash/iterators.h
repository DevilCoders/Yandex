#pragma once

#include <util/system/defaults.h>
#include <util/stream/file.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/charset/wide.h>

#include <iterator>

class TKeysContainer {
    template <typename KK>
    class TIteratorBase: public std::iterator<std::forward_iterator_tag, KK> {
    public:
        typedef typename TIteratorBase::difference_type TDiff;
        typedef typename TIteratorBase::value_type TVal;
        typedef typename TIteratorBase::reference TRef;
        typedef typename TIteratorBase::pointer TPtr;
        typedef TIteratorBase<KK> TThis;

    public:
        TIteratorBase()
            : Inp_(nullptr)
            , Wide_(false)
        {
        }

        TIteratorBase(const TString& fileName, std::function<TStringBuf(TStringBuf)> transform, bool wide = false)
            : FileName_(fileName)
            , Inp_(OpenInput(fileName))
            , Wide_(wide)
            , Transform_(transform)
        {
            Advance(1);
        }

        TIteratorBase(const TString& fileName, bool wide = false)
            : FileName_(fileName)
            , Inp_(OpenInput(fileName))
            , Wide_(wide)
        {
            Advance(1);
        }

        void Swap(TThis& other) {
            DoSwap(FileName_, other.FileName_);
            DoSwap(Inp_, other.Inp_);
            DoSwap(Buf_, other.Buf_);
            DoSwap(Val_, other.Val_);
            DoSwap(Wide_, other.Wide_);
        }

        TIteratorBase(const TThis& other)
            : FileName_(other.FileName_)
            , Inp_(other.Inp_)
            , Buf_(other.Buf_)
            , Val_(other.Val_)
            , Wide_(other.Wide_)
        {
        }

        TThis& operator=(const TThis& other) {
            TThis tmp(other);
            DoSwap(*this, tmp);
            return *this;
        }

        TThis& operator++() {
            return *this += 1;
        }

        TThis operator++(int) {
            TThis tmp(*this);
            ++(*this);
            return tmp;
        }

        TThis& operator+=(TDiff n) {
            Advance(n);
            return *this;
        }

        TThis operator+(TDiff n) {
            TThis result(*this);
            return result += n;
        }

        const TRef operator*() const {
            return Val_;
        }

        const TPtr operator->() const {
            return &Val_;
        }

        bool operator!=(const TThis& other) const {
            return FileName_ != other.FileName_ || Inp_ != other.Inp_ || Wide_ != other.Wide_;
        }

        bool operator==(const TThis& other) const {
            return !(*this != other);
        }

        const TString Line() const {
            return Buf_;
        }

    private:
        void Reset() {
            FileName_.clear();
            Inp_.Reset(nullptr);
            Buf_.clear();
            Wuf_.clear();
            Val_.clear();
            Wide_ = false;
        }

        void Advance(TDiff n) {
            if (!Inp_) {
                Reset();
                return;
            }
            for (TDiff i = 0; i < n; ++i) {
                if (Inp_->ReadLine(Buf_)) {
                    Val_.assign(Transform_(TStringBuf(Buf_).Before('\t')));
                    if (Wide_) {
                        Wuf_.AssignUtf8(Val_);
                        Val_.assign((const char*)Wuf_.data(), Wuf_.size() * sizeof(TUtf16String::TChar));
                    }

                } else {
                    Reset();
                    break;
                }
            }
        }

    private:
        TString FileName_;
        TSimpleSharedPtr<IInputStream> Inp_;
        TString Buf_;
        TString Val_;
        TUtf16String Wuf_;
        bool Wide_;
        std::function<TStringBuf(TStringBuf)> Transform_;
    };

public:
    typedef TIteratorBase<TString> TIterator;
    typedef TIteratorBase<const TString> TConstIterator;
    typedef TIterator iterator;
    typedef TConstIterator const_iterator;

    TKeysContainer(const TString& fileName, std::function<TStringBuf(TStringBuf)> transform, bool wide = false)
        : FileName_(fileName)
        , Wide_(wide)
        , Transform_(transform)
    {
    }

    TKeysContainer(const TString& fileName, bool wide = false)
        : FileName_(fileName)
        , Wide_(wide)
        , Transform_([](TStringBuf x) {return x;})
    {
    }

    TConstIterator Begin() const {
        return TConstIterator(FileName_, Transform_, Wide_);
    }

    TConstIterator End() const {
        return TConstIterator();
    }

    const_iterator begin() const {
        return Begin();
    }

    const_iterator end() const {
        return End();
    }

    ui64 Size() const {
        size_t n = 0;
        for (TConstIterator it = Begin(); it != End(); ++it, ++n)
            ;
        return n;
    }

    ui64 size() const {
        return Size();
    }

private:
    TString FileName_;
    bool Wide_;
    std::function<TStringBuf(TStringBuf)> Transform_;
};
