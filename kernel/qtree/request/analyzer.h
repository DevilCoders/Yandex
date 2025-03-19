#pragma once

struct TRequestNode;

//! rebuilds request tree moving nodes into the proper place
class TRequestAnalyzer {
public:
    TRequestNode* ProcessTree(TRequestNode* root, ui32 requestProcessingFlags);

private:
    //! changes PHRASE_PHRASE to PHRASE_USEROP for all nodes of the right subtree of operator ~~
    void PrepareTree(TRequestNode* root);

    //! removes 'minus' operator: 1. following all operators except 'and' level 2; 2. inside parentheses, quotes, zones, inpos operators and multitokens
    //! replaces 'minus' operator following 'and' level 2 operator with 'andnot' level 2 operator
    //! @note removing minus ops (in some cases) intended to eliminate incorrect interpretation of multitokens with spaces: [sankt -petersburg hotels]
    TRequestNode* ProcessMinusOps(TRequestNode* root);

    //! sets WildCard flags in these cases:
    // node*   WILDCARD_PREFIX
    // *node   WILDCARD_SUFFIX
    // *node*  WILDCARD_INFIX
    TRequestNode* ProcessWildCards(TRequestNode* root);
};
