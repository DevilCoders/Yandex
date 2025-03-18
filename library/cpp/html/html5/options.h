#pragma once

namespace NHtml5 {
    struct TParserOptions {
        // If true close <plaintext> and continue parse document as ordinal html.
        bool CompatiblePlainText = false;
        // If true parse <noscript> as ordinal tag.
        bool EnableScripting = false;
        // If true set w0 for tokens
        bool EnableNoindex = false;
        // If true convert <noindex> to comment token on tokenization level.
        bool NoindexToComment = false;
        // Set all text pointers to the source text buffer.
        bool PointersToOriginal = false;
    };

}
