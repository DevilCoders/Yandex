#pragma once

namespace NTokenLogic {

// Version history:
// 1 - initial version
// 2 - allow specifying list of languages in lang and qlang attributes (rev. 920473)
// 3 - add support of agreements to token-logic (rev. 935087)
// 4 - distance agreement (rev. 936091)
// 5 - literal format change (marker agreement) (rev. HEAD)
// 6 - additional literal properties for specifying symbol categories and special cs-1upper case property (rev. 984090)
// 7 - ireg - case-insensitive regular expressions in logical conditions (rev. 987657)
// 8 - lemq - lemma quality in logical conditions (rev. 990714)
// 9 - Regular expressions for logical conditions' attributes, Ftext attribute for fulltext matching (rev. 1001821)
// 10 - Rule weight setting support (rev. 1021033)
// 11 - Property "user" (rev. 1026298)
// 12 - XOR operator in tokenlogic rules (rev. 1050278)
// 13 - Agreement by text (rev. 1074322)
// 14 - Agreement by gazetteer id (rev. 1082224)
// 15 - Heavy rule flag (rev. 1089787)
// 16 - Properties 'first' and 'last' (rev. 1094101)
// 17 - Ftext attribute changed to text, added ntext attribute for normalized text matching (rev. 1146135)
// 18 - len literal attribute for checking text length (rev. 1327632)
// 19 - Property 'punct' (rev. 1430145)
// 20 - Split the `Op` logic instruction into three separate opcodes (rev. HEAD)
const static ui16 TLOGIC_BINARY_VERSION = 20;

} // NTokenLogic
