#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/entity/htmlentity.h>

#include <util/memory/tempbuf.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace {
    enum EDataType {
        DT_NODE,
        DT_TEXT,
    };

    TString Space(size_t len) {
        return TString(len, ' ');
    }

    TString EventText(const THtmlChunk& e, EDataType type) {
        switch (type) {
            case DT_NODE: {
                TString ret = "<";

                switch (e.Namespace) {
                    case ETagNamespace::HTML:
                        break;
                    case ETagNamespace::SVG:
                        ret += "svg ";
                        break;
                    case ETagNamespace::MATHML:
                        ret += "math ";
                        break;
                }

                return ret + ((e.Tag->id() == HT_any) ? to_lower(TString(GetTagName(e))) : TString(e.Tag->lowerName)) + ">";
            }
            case DT_TEXT: {
                if (!e.text || !e.leng)
                    return TString();

                TTempBuf buf(e.leng * 4);
                size_t len = HtEntDecodeToUtf8(CODES_UTF8, e.text, e.leng, buf.Data(), e.leng * 4);

                return TString(buf.Data(), len);
            }
        }
        return TString();
    }

    class TBuilder: public IParserResult {
    public:
        TBuilder(IOutputStream& out)
            : Output_(out)
            , Level_(0)
        {
        }

        ~TBuilder() override {
            Y_ASSERT(Level_ == 0);
        }

        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override {
            switch (chunk.GetLexType()) {
                case HTLEX_MD:
                    Output_ << "| <!DOCTYPE html>" << '\n';
                    break;
                case HTLEX_START_TAG:
                case HTLEX_EMPTY_TAG: {
                    Output_ << "| " << Space(Level_) << EventText(chunk, DT_NODE) << '\n';

                    TVector<std::pair<TString, EAttrNS>> data;

                    for (size_t i = 0; i < chunk.AttrCount; ++i) {
                        const NHtml::TAttribute& attr = chunk.Attrs[i];
                        TTempBuf buf(attr.Value.Leng);

                        size_t len = HtDecodeAttrToUtf8(CODES_UTF8, chunk.text + attr.Value.Start, attr.Value.Leng,
                                                        buf.Data(), attr.Value.Leng);

                        data.push_back(std::make_pair((to_lower(TString(chunk.text + attr.Name.Start, attr.Name.Leng)) + "=\"" + (attr.IsBoolean() ? TString() : TString(buf.Data(), len)) + "\""),
                                                      attr.Namespace));
                    }

                    Sort(data.begin(), data.end());

                    for (size_t i = 0; i < data.size(); ++i) {
                        Output_ << "| " << TString(Level_ + 2, ' ');

                        switch (data[i].second) {
                            case EAttrNS::NONE:
                                break;
                            case EAttrNS::XLINK:
                                Output_ << "xlink ";
                                break;
                            case EAttrNS::XML:
                                Output_ << "xml ";
                                break;
                            case EAttrNS::XMLNS:
                                Output_ << "xmlns ";
                                break;
                        }

                        Output_ << data[i].first << '\n';
                    }
                } break;
                case HTLEX_TEXT:
                    Output_ << "| " << Space(Level_) << "\"" << EventText(chunk, DT_TEXT) << "\"" << '\n';
                    break;
                case HTLEX_COMMENT:
                    Output_ << "| " << Space(Level_) << EventText(chunk, DT_TEXT) << '\n';
                    break;
                default:
                    break;
            }

            Output_.Flush();

            if (chunk.GetLexType() == HTLEX_START_TAG) {
                Level_ += 2;
            } else if (chunk.GetLexType() == HTLEX_END_TAG) {
                Level_ -= 2;
            }

            return nullptr;
        }

    private:
        IOutputStream& Output_;
        int Level_;
    };

}

void PrintChunkTree(const TStringBuf& html, IOutputStream& out) {
    TBuilder build(out);
    NHtml5::ParseHtml(html, &build, TStringBuf());
}
