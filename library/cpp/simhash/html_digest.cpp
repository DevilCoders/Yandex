#include "html_digest.h"

#include <library/cpp/html/face/blob/chunkslist.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/spec/lextype.h>

#include <util/string/strip.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <utility>
#include <library/cpp/digest/md5/md5.h>

class THtmlDigestCalc: public IParserResult {
    struct TCanonChunk {
        const THtmlChunk* Chunk;
        TStringBuf Text;
        TVector<std::pair<TStringBuf, TStringBuf>> Attrs;

        bool Apply(const THtmlChunk& chunk) {
            Chunk = &chunk;
            Text.Clear();
            Attrs.clear();

            if (!IsUsefull()) {
                return false;
            }

            {
                auto begin = chunk.text;
                auto end = chunk.text + chunk.leng;

                StripRange(begin, end);
                Text = TStringBuf(begin, end);
            }
            {
                for (size_t i = 0; i < chunk.AttrCount; ++i) {
                    const NHtml::TAttribute& attr = chunk.Attrs[i];
                    TStringBuf name;
                    TStringBuf value;
                    {
                        auto begin = &chunk.text[attr.Name.Start];
                        auto end = begin + attr.Name.Leng;

                        name = TStringBuf(begin, end);
                    }
                    if (!attr.IsBoolean()) {
                        auto begin = &chunk.text[attr.Value.Start];
                        auto end = begin + attr.Value.Leng;

                        StripRange(begin, end);
                        value = TStringBuf(begin, end);
                    }
                    Attrs.push_back(std::make_pair(name, value));
                }
                ::Sort(Attrs);
            }
            return true;
        }

        bool IsUsefull() const {
            if (Chunk->GetLexType() == HTLEX_COMMENT) {
                return false;
            }
            if ((MARKUP_TYPE)Chunk->flags.markup != MARKUP_NORMAL) {
                return false;
            }
            if (!Chunk->text || Chunk->IsWhitespace) {
                return false;
            }
            return true;
        }

        void UpdateStrongMD5(MD5& md5) {
            //Cerr << "---------------------------------------" << Endl;
            //Cerr << Text << Endl;
            if (Chunk->flags.type == PARSED_MARKUP) {
                TStringBuf tag(Chunk->Tag->lowerName ? Chunk->Tag->lowerName : Chunk->Tag->name);
                if (Chunk->GetLexType() == HTLEX_END_TAG) {
                    md5.Update("/", 1);
                }
                md5.Update(tag.data(), tag.size());
                md5.Update(" ", 1);

                //Cerr << tag << Endl;

                for (const auto& attr : Attrs) {
                    md5.Update(attr.first.data(),
                               attr.first.size());

                    //Cerr << attr.first << " = " << attr.second << Endl;

                    if (attr.second.size()) {
                        md5.Update("=", 1);
                        md5.Update(attr.second.data(),
                                   attr.second.size());
                    }
                    md5.Update("\n", 1);
                }
            } else if (Chunk->flags.type == PARSED_TEXT) {
                if (Text.size()) {
                    md5.Update(Text.data(), Text.size());
                }
                md5.Update("\n", 1);
            }
            //Cerr << "---------------------------------------" << Endl;
        }

        void UpdateWeakMD5(MD5& md5) {
            if (Chunk->flags.type == PARSED_MARKUP && Chunk->GetLexType() == HTLEX_START_TAG) {
                if (Chunk->Tag->id() == HT_IMG) {
                    for (const auto& attr : Attrs) {
                        if (attr.first == "src") {
                            md5.Update(attr.second.data(),
                                       attr.second.size());
                            md5.Update("\n", 1);
                        }
                    }
                }
            } else if (Chunk->flags.type == PARSED_TEXT && Chunk->flags.weight != WEIGHT_ZERO) {
                if (Text.size()) {
                    md5.Update(Text.data(), Text.size());
                }
                md5.Update("\n", 1);
            }
        }
    };

public:
    THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override {
        if (Chunk.Apply(chunk)) {
            Chunk.UpdateStrongMD5(StrongDigest);
            Chunk.UpdateWeakMD5(WeakDigest);
        }
        return nullptr;
    }

    THtmlDigest GetDigest() {
        THtmlDigest result;
        {
            result.StrongDigest.resize(32);
            StrongDigest.End(result.StrongDigest.begin());
        }
        {
            result.WeakDigest.resize(32);
            WeakDigest.End(result.WeakDigest.begin());
        }
        return result;
    }

private:
    TCanonChunk Chunk;
    MD5 StrongDigest;
    MD5 WeakDigest;
};

THtmlDigest GetHtmlDigest(TStringBuf html) {
    THtmlDigestCalc digest;
    NHtml5::ParseHtml(html, &digest);
    return digest.GetDigest();
}
