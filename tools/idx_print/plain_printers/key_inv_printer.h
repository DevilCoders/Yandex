#pragma once

#include <kernel/doom/algorithm/transfer.h>
#include <kernel/doom/key/old_key_decoder.h>
#include <kernel/doom/key/decoded_key.h>
#include <kernel/doom/yandex/yandex_io.h>
#include <kernel/doom/offroad/offroad_panther_io.h>
#include <kernel/doom/offroad/offroad_counts_io.h>
#include <kernel/doom/offroad_double_panther_wad/offroad_double_panther_wad_io.h>
#include <kernel/doom/offroad_attributes_wad/offroad_attributes_wad_io.h>
#include <kernel/doom/info/detect_index_format.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>

#include <tools/idx_print/utils/options.h>

enum EKeyAction {
    WriteKeyAction,
    SkipKeyAction,
    StopKeyAction
};

template<class Io>
struct TIdentityFunctions {
    class TReader : public Io::TReader {
        using TBase = typename Io::TReader;
    public:
        TReader(const TIdxPrintOptions& options)
            : TBase(options.IndexPath)
        {
            if (options.Query) {
                TBase::LowerBound(options.Query);
            }
        }
    };

    static EKeyAction KeyAction(const TStringBuf& key, const TIdxPrintOptions& options) {
        if (options.ExactQuery) {
            return key == options.Query ? WriteKeyAction : StopKeyAction;
        } else {
            return key.StartsWith(options.Query) ? WriteKeyAction : StopKeyAction;
        }
    }
};

void EscapeNonAsciiChars(IOutputStream& out, TStringBuf s) {
    for (unsigned char ch : s) {
        if (ch < 128) {
            out << ch;
        } else {
            out << "\\x" << HexEncode(&ch, 1);
        }
    }
}

template <class Key>
void DecodeAndPrintKey(const Key& key, const TIdxPrintOptions& options) {
    NDoom::TOldKeyDecoder decoder;
    NDoom::TDecodedKey tempKey;
    if (options.YandexEncoded) {
        if (decoder.Decode(key, &tempKey)) {
            Cout << tempKey << "\n";
        } else {
            if (!options.HexEncodeBadChars) {
                Cout << key << " (encoding-error)\n";
            } else {
                TString s;
                TStringOutput tmp(s);
                tmp << key;
                EscapeNonAsciiChars(Cout, s);
                Cout << "\n";
            }
        }
    } else {
        Cout << key << "\n";
    }
}

template<class Functions>
void PrintKeyInvIndex(const TIdxPrintOptions& options) {
    typename Functions::TReader reader(options);

    typename Functions::TReader::TKeyRef key;
    typename Functions::TReader::THit hit;

    while (reader.ReadKey(&key)) {
        EKeyAction action = Functions::KeyAction(key, options);
        if (action == StopKeyAction) {
            break;
        } else if (action == SkipKeyAction) {
            continue;
        }

        if (options.DocIds.empty()) {
            DecodeAndPrintKey(key, options);
            if (options.PrintHits) {
                if (Functions::TReader::HitLayers > 1) {
                    for (size_t layer = 1; NDoom::NextLayer(reader); ++layer) {
                        Cout << "Layer #" << layer << "\n";
                        while (reader.ReadHit(&hit)) {
                            Cout << "\t" << hit << "\n";
                        }
                    }
                } else {
                    while (reader.ReadHit(&hit)) {
                        Cout << "\t" << hit << "\n";
                    }
                }
            }
        } else {
            Y_ENSURE(options.PrintHits);

            bool keyPrinted = false;

            if (Functions::TReader::HitLayers > 1) {
                for (size_t layer = 1; NDoom::NextLayer(reader); ++layer) {
                    bool layerPrinted = false;
                    while (reader.ReadHit(&hit)) {
                        if (options.DocIds.contains(hit.DocId())) {
                            if (!keyPrinted) {
                                DecodeAndPrintKey(key, options);
                                keyPrinted = true;
                            }
                            if (!layerPrinted) {
                                Cout << "Layer #" << layer << "\n";
                                layerPrinted = true;
                            }

                            Cout << "\t" << hit << "\n";
                        }
                    }
                }
            } else {
                while (reader.ReadHit(&hit)) {
                    if (options.DocIds.contains(hit.DocId())) {
                        if (!keyPrinted) {
                            DecodeAndPrintKey(key, options);
                            keyPrinted = true;
                        }

                        Cout << "\t" << hit << "\n";
                    }
                }
            }
        }
    }
}
