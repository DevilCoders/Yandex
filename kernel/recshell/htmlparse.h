#pragma once

#include <dict/recognize/docrec/recinpbuffer.h>
#include <dict/recognize/docrec/recognizer.h>

#include <library/cpp/html/storage/queue.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NHtml {
    class TChunksRef;
}

namespace NRecognizerShell {

/// Hints for the recognizer that can be extracted from HTML metadata.
struct TMetaTagsHints {
    TString Language;
    TString Encoding;
};


/// Parse the HTML code choosing the proper parser basing on the url.
 /* @param[in] hints Documents hints that are used to choose parser.
  * @param[out] buffer The buffer that is used to store the result. The
  * 'recognizerBuffer' arguments uses this buffer for storage.
  * @param[out] recognizerBuffer The positions in the 'buffer' argument that
  * determine the parsing results. @note this argument does not need to be
  * initialized as it is initialized inside this function.
  * @param[out] metaTagsHints The hints that were found in the HTML code.
  * @param[in] generalParserOnly This parameter can be used to disable
  * custom parsing for specific hosts.
  */
void ParseHtml(const TStringBuf& data,
               const TRecognizer::THints& hints,
               TVector<wchar16>* buffer,
               TRecognizeInputBuffer* recognizerBuffer,
               TMetaTagsHints* metaTagsHints,
               bool unknownUtf = false,
               bool generalParserOnly = false);

void ParseHtml(NHtml::TSegmentedQueueIterator first,
               NHtml::TSegmentedQueueIterator last,
               const TRecognizer::THints& hints,
               TVector<wchar16>* buffer,
               TRecognizeInputBuffer* recognizerBuffer,
               TMetaTagsHints* metaTagsHints,
               bool unknownUtf = false,
               bool generalParserOnly = false);

void ParseHtml(const NHtml::TChunksRef& chunks,
               const TRecognizer::THints& hints,
               TVector<wchar16>* buffer,
               TRecognizeInputBuffer* recognizerBuffer,
               TMetaTagsHints* metaTagsHints,
               bool unknownUtf = false,
               bool generalParserOnly = false);

}
