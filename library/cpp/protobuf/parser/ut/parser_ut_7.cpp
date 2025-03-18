#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest7() {
        NParserUt::TTestMessage proto;
        NProtoParser::TMessageParser msg(proto);

        //
        // message
        //

        UNIT_ASSERT_EXCEPTION(msg.Merge("optional_message", TString("msg")), yexception);
    }
}
