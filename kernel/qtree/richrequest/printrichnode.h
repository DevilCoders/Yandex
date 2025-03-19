#pragma once

#include "richnode_fwd.h"

#include <util/generic/string.h>
#include <util/stream/output.h>

/*Восстанавливает по дереву TRichRequestNode строковый запрос.

Если строковый запрос распарсить, то должно получиться дерево(TRichRequestNode) равное исходному.

Известные исключения:
    2. Функция TWordNode::Init может назначить вершине другое значение FormType.
    Пример: !!"ежики в тумане" -> "!!ежики !!в тумане"

    3. Вершины-мультитокены (PhraseType == ptMultiWord) преобразуются в вершины с PhraseType == ptUserOp.
    Пример: наконец-то!! -> наконец& /(1 1) то

    5. В запросе, состоящем их вершин c IsWord() == true и одинаковым значением FormType,
    FormType может "вынестись" за скобку.
    Пример: ("Грибоедов","Дюма-мать","Дюма-сын") -> !("Грибоедов"& & "Дюма мать"& & "Дюма сын")
*/

namespace NSearchQuery {
    class TRequest;
}
class TRichRequestNode;

TUtf16String PrintRichRequest(const NSearchQuery::TRequest* n, EPrintRichRequestOptions options = PRRO_Default);
TUtf16String PrintRichRequest(const TRichRequestNode& n, EPrintRichRequestOptions options = PRRO_Default);

void PrintRichTree(const NSearchQuery::TRequest* n, IOutputStream& out);
void PrintBasicRichTree(const NSearchQuery::TRequest* n, IOutputStream& out);

// Extracts text representation of @node as it was in original query.
// If there is no associated text - collect it from children nodes.
// The node is supposed to be a single word (not the whole query).
// Restores all subnode punctuation for multitoken node if possible.
TUtf16String GetRichRequestNodeText(const TRichRequestNode* node);

// same as GetRichRequestNodeText, but returns node text in original capitalization
TUtf16String GetOriginalRichRequestNodeText(const TRichRequestNode* node);

// same as GetRichRequestNodeText with explicit lower-casing
inline TUtf16String GetLowerCaseRichRequestNodeText(const TRichRequestNode* node) {
    TUtf16String ret = GetRichRequestNodeText(node);
    ret.to_lower();     // NOTE: language independent lower-casing
    return ret;
}

// Debug output a tree structure, utf8
void DebugPrintRichTree(const TRichRequestNode& node, IOutputStream& out);
TString DebugPrintRichTree(const TRichRequestNode& node);
void PrintInfo(const TRichRequestNode& node, ui32 level = 0, ui32 syn = 0);

