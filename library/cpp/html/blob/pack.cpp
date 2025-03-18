#include "format.h"
#include "output.h"
#include "pack.h"

#include <library/cpp/html/blob/document.pb.h>
#include <library/cpp/packedtypes/longs.h>

#include <util/generic/map.h>
#include <util/memory/tempbuf.h>
#include <util/stream/str.h>

namespace NHtml {
    namespace NBlob {
        class TAttributeIterator {
        public:
            explicit TAttributeIterator(const THtmlChunk& chunk)
                : Chunk(chunk)
                , Pos(0)
            {
            }

            inline bool Boolean() const {
                return Chunk.Attrs[Pos].IsBoolean();
            }

            inline TStringBuf Name() const {
                return TStringBuf(
                    Chunk.text + Chunk.Attrs[Pos].Name.Start,
                    Chunk.Attrs[Pos].Name.Leng);
            }

            inline TStringBuf Value() const {
                if (Chunk.Attrs[Pos].IsBoolean()) {
                    return TStringBuf();
                }

                return TStringBuf(
                    Chunk.text + Chunk.Attrs[Pos].Value.Start,
                    Chunk.Attrs[Pos].Value.Leng);
            }

            inline bool Valid() const {
                return Pos < Chunk.AttrCount;
            }

            inline void Next() {
                Y_ASSERT(Valid());
                ++Pos;
            }

        private:
            const THtmlChunk& Chunk;
            size_t Pos;
        };

        class TPackEvents::TImpl {
        public:
            TImpl()
                : Output_(Data_)
                , Writer_(&Output_)
            {
                Writer_.WriteUInt8(NODE_DOCUMENT);
            }

            TString Pack() {
                TDocumentPack doc;

                Writer_.WriteUInt8(NODE_CLOSE);

                doc.SetVersion(1);
                doc.MutableMain()->SetId("main");
                doc.MutableMain()->MutableData()->SetBegin(0);
                doc.MutableMain()->MutableData()->SetEnd(Writer_.Position());

                // Save dictionaries
                Dicts_.Save(&Writer_, doc.MutableStrings());

                ui32 size = doc.ByteSize();
                TTempBuf meta(size);

                doc.SerializeWithCachedSizesToArray((ui8*)meta.Data());
                Output_.Write(meta.Data(), size);
                Output_.Write(&size, sizeof(size));
                Output_.Flush();

                return Data_;
            }

            void OnHtmlChunk(const THtmlChunk& chunk) {
                switch (chunk.GetLexType()) {
                    case HTLEX_START_TAG:
                    case HTLEX_EMPTY_TAG: {
                        ui8 flags = NODE_ELEMENT;

                        if (chunk.AttrCount) {
                            flags |= FIELD_ATTRIBUTES;
                        }

                        Writer_.WriteUInt8(flags);
                        if (chunk.Tag->id() == HT_any) {
                            Writer_.WriteVariableInt32(
                                Dicts_.GetTagId(TString(GetTagName(chunk))));
                        } else {
                            Writer_.WriteVariableInt32(
                                Dicts_.GetTagId(chunk.Tag->lowerName));
                        }

                        if (flags & FIELD_ATTRIBUTES) {
                            Writer_.WriteVariableInt32((i32)chunk.AttrCount);
                            for (TAttributeIterator ai(chunk); ai.Valid(); ai.Next()) {
                                Writer_.WriteVariableInt32(
                                    Dicts_.GetAttrNameId(TString(ai.Name())));

                                if (ai.Boolean()) {
                                    Writer_.WriteVariableInt32(0);
                                } else {
                                    Writer_.WriteVariableInt32(
                                        Dicts_.GetAttrValueId(TString(ai.Value())));
                                }
                            }
                        }

                        if (chunk.GetLexType() == HTLEX_EMPTY_TAG) {
                            Writer_.WriteUInt8(NODE_CLOSE);
                        }
                        break;
                    }

                    case HTLEX_END_TAG:
                        Writer_.WriteUInt8(NODE_CLOSE);
                        break;

                    case HTLEX_TEXT:
                        Writer_.WriteUInt8(NODE_TEXT);
                        Writer_.WriteVariableInt32(
                            Dicts_.GetTextId(TString(chunk.text, chunk.leng)));
                        break;

                    case HTLEX_COMMENT:
                        Writer_.WriteUInt8(NODE_COMMENT);
                        Writer_.WriteVariableInt32(
                            Dicts_.GetTextId(
                                TString(StipComment(TStringBuf(chunk.text, chunk.leng)))));
                        break;

                    case HTLEX_MD:
                        // TODO need to parse tag content
                        Writer_.WriteUInt8(NODE_DOCUMENT_TYPE);
                        Writer_.WriteVariableInt32(
                            Dicts_.GetTextId(TString("")));
                        break;

                    case HTLEX_EOF:
                    case HTLEX_PI:
                    case HTLEX_ASP:
                        break;
                }
            }

        private:
            TStringBuf StipComment(TStringBuf text) const {
                if (text.size() >= 4 && strnicmp(text.data(), "<!--", 4) == 0) {
                    text = TStringBuf(text.data() + 4, text.size() - 4);
                }
                if (text.size() >= 3 && strnicmp(text.data() + text.size() - 3, "-->", 3) == 0) {
                    text = TStringBuf(text.data(), text.size() - 3);
                }
                return text;
            }

        private:
            TString Data_;
            TStringOutput Output_;
            TOutput Writer_;
            TDictionaries Dicts_;
        };

        TPackEvents::TPackEvents()
            : Impl_(new TImpl)
        {
        }

        TPackEvents::~TPackEvents() {
        }

        TString TPackEvents::Pack() const {
            return Impl_->Pack();
        }

        THtmlChunk* TPackEvents::OnHtmlChunk(const THtmlChunk& chunk) {
            Impl_->OnHtmlChunk(chunk);
            return nullptr;
        }

    }
}
