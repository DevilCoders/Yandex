#pragma once

#include <library/cpp/dolbilo/plan.h>

#include <util/generic/string.h>

class IInputStream;
class IOutputStream;

class IReqsLoader {
    public:
        class IOutputter {
            public:
                inline IOutputter() noexcept {
                }

                virtual ~IOutputter() {
                }

                virtual void Add(const TDevastateItem& item) = 0;
        };

        class TParams {
            public:
                inline TParams(IInputStream* in, IOutputter* out)
                    : Input_(in)
                    , Outputter_(out)
                {
                }

                inline void Add(const TDevastateItem& item) {
                    Outputter_->Add(item);
                }

                inline IInputStream* Input() const noexcept {
                    return Input_;
                }

                inline IOutputter* Outputter() const noexcept {
                    return Outputter_;
                }

            private:
                IInputStream* Input_;
                IOutputter* Outputter_;
        };

        struct TOption {
            char key;
            const char* opt;
        };

        inline IReqsLoader() noexcept {
        }

        virtual ~IReqsLoader() {
        }

        virtual bool HandleOpt(const TOption* /*option*/) {
            return false;
        }

        virtual TString Opts() {
            return TString();
        }

        virtual void Usage() const {
        }

        virtual void Process(TParams* params) = 0;
        virtual IReqsLoader* Clone() const = 0;
};
