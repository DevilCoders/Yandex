#include "qd_key_token.h"

namespace NQueryData {

    template <class Op>
    static void ProcessKeySubkeys(TStringBuf body, Op op) {
        while (true) {
            TStringBuf left, right;
            if (body.TrySplit('\t', left, right)) {
                op(left);
                body = right;
                if (!body) {
                    op(body);
                    break;
                }
            } else {
                op(body);
                break;
            }
        }
    }

    TSubkeysCounts GetAllSubkeysCounts(TStringBuf q) {
        TSubkeysCounts cnts;
        ProcessKeySubkeys(q, [&cnts](TStringBuf sk) {
            bool empty = sk.empty();
            cnts.Empty += empty;
            cnts.Nonempty += !empty;
        });
        return cnts;
    }

    void SplitKeyIntoSubkeys(TStringBuf body, TStringBufs& subkeys) {
        subkeys.clear();
        ProcessKeySubkeys(body, [&subkeys](TStringBuf k) {
            subkeys.push_back(k);
        });
    }

}
