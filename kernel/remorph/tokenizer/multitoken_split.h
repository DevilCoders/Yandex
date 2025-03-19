#pragma once

namespace NToken {

// Multitokens post-processing mode regulates how multitokens are split up (if split at all) after tokenization.
enum EMultitokenSplit {
    MS_MINIMAL, // Split only by minimal set of characters required for sane operation (default).
    MS_SMART,   // Split all multitokens, except tokens-with-hyphen, tokens'with'apostrophe composed of words.
    MS_ALL,     // Split all multitokens.
};

} // NToken
