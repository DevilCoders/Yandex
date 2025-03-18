#include "minhash_helpers.h"

namespace NMinHash {
    const ui64 TChdMinHashFuncHdr::SIGNATURE = 0x68736148646843; // ChdHash
    const ui8 TChdMinHashFuncHdr::VERSION = 1;

    const ui64 TTableHdr::SIGNATURE = 0x6c6254646843; // ChdTbl
    const ui8 TTableHdr::VERSION = 1;

}
