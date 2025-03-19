#include "grammar_index.h"

#include <util/generic/singleton.h>

// Singleton object to hold all indexes
const TGrammarIndex& TGrammarIndex::TheIndex() {
    return *Singleton<TGrammarIndex>();
}
