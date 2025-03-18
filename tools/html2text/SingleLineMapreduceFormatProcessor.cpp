#include "HtmlParser.h"
#include "SingleLineMapreduceFormatProcessor.h"

TransformTables SingleLineMapreduceFormatProcessor::Transforms;

TransformTables::TransformTables() {
    for (int i = 0; i < 256; i++) {
        transformFromMRTable[i] = i;
        transformToMRTable[i] = i;
    }
    transformFromMRTable[(unsigned char)'\x05'] = '\t';
    transformFromMRTable[(unsigned char)'\x06'] = '\n';

    transformToMRTable[(unsigned char)'\x05'] = ' ';
    transformToMRTable[(unsigned char)'\x06'] = ' ';
    transformToMRTable[(unsigned char)'\t'] = '\x05';
    transformToMRTable[(unsigned char)'\n'] = '\x06';
}

void TransformTables::TransformString(const char(&table)[256], char* str, size_t length) const {
    for (size_t i = 0; i < length; ++i) {
        str[i] = table[(unsigned char) str[i]];
    }
}

void TransformTables::TransformString(const char(&table)[256], TString str) const {
    TransformString(table, const_cast<char*>(str.data()), str.size());
}

void TransformTables::TransformFromMR(TString str) const {
    TransformString(transformFromMRTable, str);
}

void TransformTables::TransformToMR(TString str) const {
    TransformString(transformToMRTable, str);
}


void SingleLineMapreduceFormatProcessor::ProcessHtml(IInputStream& input, IOutputStream& output) {
    TString record;
    while (input.ReadTo(record, '\n')) {
        size_t firstSeparatorIdx = record.find('\t');
        size_t lastSeparatorIdx = record.rfind('\t');
        if (firstSeparatorIdx == TString::npos) {
            TString errorText = "Bad record format: '" + record + "'";
            ythrow yexception() << errorText.data();
        }
        TString key = record.substr(0, firstSeparatorIdx + 1);
        TString html = record.substr(lastSeparatorIdx + 1);
        Transforms.TransformFromMR(html);

        TSimpleSharedPtr<TTextAndTitleSentences> sentences = Html2Text(html);
        TString titleString = WideToUTF8(sentences->GetTitle());
        TString textString = WideToUTF8(sentences->GetText(WideNewLine));

        Transforms.TransformToMR(titleString);
        Transforms.TransformToMR(textString);

        output << key << '\t' << titleString << '\t' << textString << '\n';
    }
}
