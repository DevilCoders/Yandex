#pragma once

namespace NReMorph {

// Version history:
// 1 - initial version (was not used in the binary format)
// 2 - added support of text-case properties (rev. 455873)
// 3 - fix of platform-dependent serialization (rev. 539945)
// 4 - added support of remorph cascades (rev. 569142)
// 5 - allow specifying list of gazetteer items in the single logical expression (rev. 707496)
// 6 - resolving gazetteer ambiguity during remorph match (rev. 735016)
// 7 - alang property has been removed (rev. 766613)
// 8 - rule priorities have been added (rev. 787675)
// 9 - DFA internal structure has been changed with the support of multiple final IDs in the single state (rev. 846370)
// 10 - Integrate tagger into remorph (rev. 852236)
// 11 - Add support of article field comparison in logic expressions (rev. 856244)
// 12 - Change DFA memory representation and serialization (rev. 859162)
// 13 - Minimal path length in DFA (rev. 883914)
// 14 - Set of lemmas (rev. 900584)
// 15 - Move cascades to separate lib (rev. 917061)
// 16 - allow specifying list of languages in lang and qlang attributes (rev. 920473)
// 17 - distance agreement (rev. 936091)
// 18 - marker agreement (rev. 937918)
// 19 - additional literal properties for specifying symbol categories and special cs-1upper case property (rev. 984090)
// 20 - ireg - case-insensitive regular expressions in logical conditions (rev. 987657)
// 21 - lemq - lemma quality in logical conditions (rev. 990714)
// 22 - Regular expressions for logical conditions' attributes, Ftext attribute for fulltext matching (rev. 1001821)
// 23 - Property "user" (rev. 1026298)
// 24 - Agreement by text (rev. 1074322)
// 25 - Agreement by gazetteer id (rev. 1082224)
// 26 - Properties 'first' and 'last' (rev. 1094101)
// 27 - Ftext attribute changed to text, added ntext attribute for normalized text matching (rev. 1146135)
// 28 - len literal attribute for checking text length (rev. 1327632)
// 29 - Property 'punct' (rev. 1430145)
// 30 - Split the `Op` logic instruction into three separate opcodes (rev. HEAD)
const static ui16 REMORPH_BINARY_VERSION = 30;

} // NReMorph
