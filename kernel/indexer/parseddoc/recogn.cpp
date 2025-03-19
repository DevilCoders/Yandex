#include "recogn.h"

#include <kernel/indexer/face/docinfo.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <kernel/recshell/recshell.h>
#include <kernel/tarc/iface/fulldoc.h>

ERecognRes PrepareEncodingAndLanguage(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
                                      IParsedDocProperties* docProps, TRecognizerShell* recogn, const TDocInfoEx* docInfo) {
    TFullArchiveDocHeader* docHeader = docInfo->DocHeader;
    ERecognRes res = RR_PREDEFINED;
    ELanguage lang = (ELanguage)docHeader->Language;
    ELanguage lang2 = (ELanguage)docHeader->Language2;
    ECharset dc = (ECharset)docHeader->Encoding;

    if (dc == CODES_UNKNOWN || lang == LANG_UNK) {
        const char* prop = nullptr;
        if (dc == CODES_UNKNOWN) {
            if (docProps->GetProperty(PP_CHARSET, &prop) == 0)
                dc = CharsetByName(prop);
        }
        if (lang == LANG_UNK) {
            if (docProps->GetProperty(PP_LANGUAGE, &prop) == 0)
                lang = LanguageByName(prop);
        }
        if (dc == CODES_UNKNOWN || lang == LANG_UNK) {
            if (recogn) {
                TRecognizer::THints hints;
                hints.HttpCodepage = dc;
                recogn->Recognize(first, last, &dc, &lang, &lang2, hints);
                if (lang == LANG_UNK || lang == LANG_UNK_LAT || lang == LANG_UNK_CYR || lang == LANG_UNK_ALPHA) {
                    if (docProps->GetProperty(PP_DEFLANGUAGE, &prop) == 0)
                        lang = LanguageByName(prop);
                }
                if (lang == LANG_UNK_LAT || lang == LANG_UNK_CYR || lang == LANG_UNK_ALPHA) {
                    docProps->SetProperty(PP_INDEXRES, "The charset is not recognized");
                    return RR_UNKNOWN;
                }
                res = RR_RECOGNIZED;
            }
        }
        if (dc == CODES_UNKNOWN) {
            if (docProps->GetProperty(PP_DEFCHARSET, &prop) == 0)
                dc = CharsetByName(prop);
            if (dc == CODES_UNKNOWN) {
                if (first != last && GetHtmlChunk(first)->flags.type == PARSED_EOF)
                    dc = CODES_WIN;
                if (dc == CODES_UNKNOWN) {
                    docProps->SetProperty(PP_INDEXRES, "The charset is not recognized");
                    return RR_UNKNOWN;
                }
            }
        }
        docHeader->Language = (ui8)lang;
        docHeader->Language2 = (ui8)lang2;
        docHeader->Encoding = (i8)dc;
    }
    if (docInfo->IsLogical)
        docProps->SetProperty(PP_CHARSET, NameByCharset(CODES_UTF8));
    else
        docProps->SetProperty(PP_CHARSET, NameByCharset((ECharset)docHeader->Encoding));
    docProps->SetProperty(PP_LANGUAGE, NameByLanguage((ELanguage)docHeader->Language));
    return res;
}
