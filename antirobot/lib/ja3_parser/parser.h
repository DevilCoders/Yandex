#pragma once

#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/string/vector.h>
#include <util/generic/hash.h>

namespace Ja3Parser {
    struct TJa3 {
        float TlsVersion = 0;
        float CiphersLen = 0;
        float ExtensionsLen = 0;

        float C159 = 0;
        float C57_61 = 0;
        float C53 = 0;
        float C60_49187 = 0;
        float C53_49187 = 0;
        float C52393_103 = 0;
        float C49162 = 0;
        float C50 = 0;
        float C51 = 0;
        float C255 = 0;
        float C52392 = 0;
        float C10 = 0;
        float C157_49200 = 0;
        float C49200 = 0;
        float C49171_103 = 0;
        float C49191_52394 = 0;
        float C49192_52394 = 0;
        float C65_52394 = 0;
        float C157 = 0;
        float C52393_49200 = 0;
        float C49159 = 0;
        float C4865 = 0;
        float C158_61 = 0;
        float C49196_47 = 0;
        float C103 = 0;
        float C103_49196 = 0;
        float C52393_49188 = 0;
        float C60_65 = 0;
        float C49195_69 = 0;
        float C154 = 0;
        float C49187_49188 = 0;
        float C49199_60 = 0;
        float C49195_51 = 0;
        float C49235 = 0;
        float C47 = 0;
        float C49169 = 0;
        float C49249 = 0;
        float C49171_60 = 0;
        float C49188_49196 = 0;
        float C61 = 0;
        float C156_255 = 0;
        float C47_57 = 0;
        float C186 = 0;
        float C49245 = 0;
        float C156_52394 = 0;
        float C20 = 0;
        float C49188_49195 = 0;
        float C52394_52392 = 0;
        float C53_49162 = 0;
        float C49191 = 0;
        float C49245_49249 = 0;
        float C49171 = 0;
        float C53_52393 = 0;
        float C51_49199 = 0;
        float C49234 = 0;
        float C49315 = 0;
        float C158 = 0;
        float C49187_49161 = 0;
        float C107 = 0;
        float C52394 = 0;
        float C49162_61 = 0;
        float C153 = 0;
        float C49170 = 0;
        float C156 = 0;
        float C52393_60 = 0;
        float C49195_49192 = 0;
        float C7 = 0;
        float C49187_103 = 0;
        float C61_49172 = 0;
        float C159_49188 = 0;
        float C49171_49187 = 0;
        float C49196_49188 = 0;
        float C158_49161 = 0;
        float C49193 = 0;

        void SetFactor(TStringBuf key, float value);
    };

    TJa3 StringToJa3(TStringBuf buf);
} // namespace Ja3Parser
