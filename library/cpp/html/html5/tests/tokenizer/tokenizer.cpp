#include <library/cpp/html/html5/tests/common/test.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/html/entity/htmlentity.h>

#include <util/charset/utf8.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>

#include <library/cpp/html/html5/tokenizer.h>
#include <library/cpp/html/html5/ascii.h>

//    OUTPUT FORMAT:
//
//    ["DOCTYPE", name, public_id, system_id, correctness]
//    ["StartTag", name, {attributes}*, true*]
//    ["StartTag", name, {attributes}]
//    ["EndTag", name]
//    ["Comment", data]
//    ["Character", data]
//    "ParseError"

class TTokenizerTest: public ITest {
public:
    TTokenizerTest(int argc, const char** argv)
        : ITest(argc, argv)
    {
    }

private:
    typedef TVector<TString> TTokens;

    void ProcessTestsFile(const TString& path) override {
        TFileInput input(path);
        NJson::TJsonReaderConfig config;
        config.DontValidateUtf8 = true;
        NJson::TJsonValue jsonValue;
        NJson::ReadJsonTree(&input, &config, &jsonValue, true);

        if (jsonValue.IsMap()) {
            TString testsKey;
            if (jsonValue.Has("tests")) {
                testsKey = "tests";
            }
            if (jsonValue.Has("xmlViolationTests")) {
                testsKey = "xmlViolationTests";
            }

            if (!testsKey.empty()) {
                const NJson::TJsonValue& tests = jsonValue[testsKey.data()];
                for (ui32 i = 0; i < tests.GetArray().size(); ++i) {
                    const TString input2 = tests[i]["input"].GetString();
                    ProcessOneDoc(input2);
                    Cout << Endl << Endl;
                }
            }
        }
    }

    void ProcessSingleDoc(const TString& input) override {
        ProcessOneDoc(input);
    }

    inline static char DigitToChar(unsigned char digit) {
        if (digit < 10) {
            return (char)digit + '0';
        }

        return (char)(digit - 10) + 'a';
    }

    TString EncodeUtf16(wchar32 rune) const {
        TString ret;
        if (rune < 0x10000) {
            ui8* ptr = (ui8*)&rune;
            ret += TString("\\u") + DigitToChar(ptr[1] / 16) + DigitToChar(ptr[1] % 16) + DigitToChar(ptr[0] / 16) + DigitToChar(ptr[0] % 16);
        } else {
            rune -= 0x10000;
            ui16 first = 0xd800 | (rune >> 10);
            ui8* ptr = (ui8*)&first;
            ret += TString("\\u") + DigitToChar(ptr[1] / 16) + DigitToChar(ptr[1] % 16) + DigitToChar(ptr[0] / 16) + DigitToChar(ptr[0] % 16);

            ui16 second = 0xdc00 | (0x03ff & (rune << 22 >> 22));
            ptr = (ui8*)&second;
            ret += TString("\\u") + DigitToChar(ptr[1] / 16) + DigitToChar(ptr[1] % 16) + DigitToChar(ptr[0] / 16) + DigitToChar(ptr[0] % 16);
        }

        return ret;
    }

    TString ReplaceQuote(const TString& s) const {
        TString ret;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\n') {
                ret += "\\n";
            } else if (s[i] == '\r') {
                ret += "\\r";
            } else if (s[i] == '\b') {
                ret += "\\b";
            } else if (s[i] == '\f') {
                ret += " ";
            } else if (s[i] == '\t') {
                ret += "\\t";
            } else if (s[i] == '"') {
                ret += "\\\"";
            } else if (s[i] == '\\') {
                ret += "\\\\";
            } else {
                ret += s[i];
            }
        }
        return ret;
    }

    TString EncodeRune(wchar32 c) const {
        if (c >= 0x20 && c < 128 || (c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D))
            return ReplaceQuote(TString() + char(c));
        else
            return EncodeUtf16(c);
    }

    TString EncodeUtf8Runes(const TString& str) const {
        TStringStream res;
        wchar32 rune;
        size_t rune_len;
        const unsigned char* cur_rune = (const unsigned char*)str.data();
        const unsigned char* end = cur_rune + str.size();

        while (SafeReadUTF8Char(rune, rune_len, cur_rune, end) == RECODE_OK) {
            res << EncodeRune(rune);
            cur_rune += rune_len;
        }
        return res.Str();
    }

    TString DecodeText(const TString& s) const {
        TBuffer buf(s.size() * 4);
        size_t len = HtEntDecodeToUtf8(CODES_UTF8, s.data(), s.size(), buf.Data(), s.size() * 4);
        return EncodeUtf8Runes(TString(buf.Data(), len));
    }

    TString ExtractTagName(const NHtml5::TStringPiece& tag) const {
        return to_lower(DecodeText(TString(tag.Data, tag.Length)));
    }

    TString ExtractAttrName(const NHtml5::TAttribute* attr) const {
        return to_lower(DecodeText(
            TString(attr->OriginalName.Data, attr->OriginalName.Length)));
    }

    TString ExtractAttrValue(const NHtml5::TAttribute* attr) const {
        if (attr->OriginalName.Data == attr->OriginalValue.Data)
            return "";

        const TString s(attr->OriginalValue.Data, attr->OriginalValue.Length);

        if (s.size() < 2)
            return s;

        TBuffer buf(s.size() * 4);

        if ((s[0] == '\'' && s[s.size() - 1] == '\'') || (s[0] == '"' && s[s.size() - 1] == '"')) {
            size_t len = HtDecodeAttrToUtf8(CODES_UTF8, s.data(), s.size(), buf.Data(), s.size() * 4);
            return EncodeUtf8Runes(TString(buf.data() + 1, len - 2));
        }

        size_t len = HtDecodeAttrToUtf8(CODES_UTF8, s.data(), s.size(), buf.Data(), s.size() * 4);
        return EncodeUtf8Runes(TString(buf.data(), len));
    }

    void FlushCharBuffer(TStringStream* charBuffer, TTokens* tokens) {
        if (charBuffer->empty())
            return;

        TStringStream tokenStr;

        tokenStr << "[\"Character\", \"" << DecodeText(charBuffer->Str()) << "\"]";
        tokens->push_back(tokenStr.Str());
        tokenStr.clear();
        charBuffer->clear();
    }

    TString ToString(const NHtml5::TStringPiece& str) const {
        return TString(str.Data, str.Length);
    }

    void ProcessOneDoc(const TString& input) {
        using namespace NHtml5;

        class TErrors: public IErrorHandler {
        public:
            void OnError(const TError& error) override {
                Errors_.push_back(error);
            }

            size_t Size() const {
                return Errors_.size();
            }

        private:
            TVector<TError> Errors_;
        };

        TErrors errs;
        TTokenizerOptions opt;
        opt.Errors = &errs;
        TTokenizer<TByteIterator> parser(opt, input.data(), input.size());

        TTokens tokens;
        TStringStream charBuffer;
        ui32 errs_count = 0;
        while (true) {
            TToken token;

            parser.Lex(&token);

            while (errs.Size() > errs_count) {
                FlushCharBuffer(&charBuffer, &tokens);
                tokens.push_back("\"ParseError\"");
                errs_count++;
            }

            if (token.Type == TOKEN_EOF) {
                break;
            }

            if (token.Type != TOKEN_CHARACTER && token.Type != TOKEN_WHITESPACE) {
                FlushCharBuffer(&charBuffer, &tokens);
            }

            TStringStream tokenStr;
            bool validToken = true;

            switch (token.Type) {
                case TOKEN_DOCTYPE: {
                    TString pub_id = token.v.DocType.HasPublicIdentifier() ? TString::Join("\"", DecodeText(ToString(token.v.DocType.PublicIdentifier)), "\"") : "null";
                    TString sys_id = token.v.DocType.HasSystemIdentifier() ? TString::Join("\"", DecodeText(ToString(token.v.DocType.SystemIdentifier)), "\"") : "null";
                    TString name = token.v.DocType.Name.Data != nullptr && token.v.DocType.Name.Data[0] ? TString::Join("\"", DecodeText(ToString(token.v.DocType.Name)), "\"") : "null";

                    tokenStr << "[\"DOCTYPE\", "
                             << name << ", "
                             << pub_id << ", "
                             << sys_id << ", "
                             << (!token.v.DocType.ForceQuirks ? "true]" : "false]");
                    break;
                }
                case TOKEN_START_TAG: {
                    if (token.v.StartTag.Tag == TAG_UNKNOWN) {
                        token.OriginalText = GetTagFromOriginalText(token.OriginalText);
                        tokenStr << "[\"StartTag\", \"" << ExtractTagName(token.OriginalText) << "\", ";
                    } else {
                        tokenStr << "[\"StartTag\", \"" << GetTagName(token.v.StartTag.Tag) << "\", ";
                    }

                    const TRange<TAttribute> attrs = token.v.StartTag.Attributes;
                    tokenStr << "{";
                    if (attrs.Length > 0) {
                        ui32 i = 0;
                        for (; i < attrs.Length - 1; ++i) {
                            TAttribute* attr = &attrs.Data[i];
                            tokenStr << "\"" << ExtractAttrName(attr) << "\": \"" << ExtractAttrValue(attr) << "\", ";
                        }

                        TAttribute* attr = &attrs.Data[i];
                        tokenStr << "\"" << ExtractAttrName(attr) << "\": \"" << ExtractAttrValue(attr) << "\"}";
                    } else {
                        tokenStr << "}";
                    }

                    if (token.v.StartTag.IsSelfClosing)
                        tokenStr << ", true]";
                    else
                        tokenStr << "]";

                    break;
                }
                case TOKEN_END_TAG: {
                    if (token.v.StartTag.Tag == TAG_UNKNOWN) {
                        token.OriginalText = GetTagFromOriginalText(token.OriginalText);
                        tokenStr << "[\"EndTag\", \"" << ExtractTagName(token.OriginalText) << "\"]";
                    } else {
                        tokenStr << "[\"EndTag\", \"" << GetTagName(token.v.EndTag) << "\"]";
                    }
                    break;
                }
                case TOKEN_COMMENT: {
                    tokenStr << "[\"Comment\", \"" << DecodeText(ToString(token.v.Text)) << "\"]";
                    break;
                }
                case TOKEN_CHARACTER:
                    charBuffer << TStringBuf(token.OriginalText.Data, token.OriginalText.Length);
                    validToken = false;
                    break;
                case TOKEN_WHITESPACE:
                case TOKEN_NULL: {
                    charBuffer << char(token.v.Character);
                    validToken = false;
                    break;
                }
                case TOKEN_EOF:
                    validToken = false;
                    break;
            }

            if (validToken)
                tokens.push_back(tokenStr.Str());
        }

        FlushCharBuffer(&charBuffer, &tokens);

        Cout << "[";
        if (tokens.size() > 0) {
            std::for_each(tokens.begin(), tokens.end() - 1, [](TString& token) { Cout << token << ", "; });
            Cout << *(tokens.end() - 1) << "]";
        } else {
            Cout << "]";
        }
    }
};

int main(int argc, const char** argv) {
    TTokenizerTest(argc, argv).Run();

    return 0;
}
