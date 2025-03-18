#include <util/generic/vector.h>

#include "sax.h"
#include "simple.h"

using namespace NXml;

namespace {
    class THandler: public ISaxHandler {
        typedef ISimpleSaxHandler::TAttr TAttr;

    public:
        inline THandler(ISimpleSaxHandler* h)
            : H_(h)
        {
        }

        void OnStartElement(const char* name, const char** a) override {
            A_.clear();

            if (a) {
                while (*a && *(a + 1)) {
                    const TAttr attr = {
                        TStringBuf(*a),
                        TStringBuf(*(a + 1)),
                    };

                    A_.push_back(attr);

                    a += 2;
                }
            }

            H_->OnStartElement(name, A_.data(), A_.size());
        }

        void OnEndElement(const char* name) override {
            H_->OnEndElement(name);
        }

        void OnText(const char* text, size_t len) override {
            H_->OnText(TStringBuf(text, len));
        }

    private:
        ISimpleSaxHandler* H_;
        TVector<TAttr> A_;
    };
}

void NXml::Parse(IInputStream& in, ISimpleSaxHandler* h) {
    THandler wrap(h);

    ParseXml(&in, &wrap);
}
