#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>

class IInputStream;

class ISlb {
    public:
        class ITranslatedHost: public TSimpleRefCount<ITranslatedHost> {
            public:
                inline ITranslatedHost() noexcept {
                }

                virtual ~ITranslatedHost() {
                }

                virtual const TString& Result() = 0;
                virtual const TString& Source() = 0;
                virtual void OnBadHost() = 0;
        };

        typedef TIntrusivePtr<ITranslatedHost> TTranslatedHostRef;

        inline ISlb() noexcept {
        }

        virtual ~ISlb() {
        }

        virtual TTranslatedHostRef Translate(const TString& host) = 0;
};

class TFakeSlb: public ISlb {
    public:
        inline TFakeSlb() noexcept {
        }

        ~TFakeSlb() override {
        }

        TTranslatedHostRef Translate(const TString& host) override;
};

class TReplaceSlb: public ISlb {
    public:
        inline TReplaceSlb(const TString& replace) noexcept
            : Replace_(replace)
        {
        }

        ~TReplaceSlb() override {
        }

        TTranslatedHostRef Translate(const TString& host) override;

    private:
        const TString Replace_;
};

class TSimpleSlb: public ISlb {
    public:
        TSimpleSlb(IInputStream* input);
        ~TSimpleSlb() override;

        TTranslatedHostRef Translate(const TString& host) override;

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};
