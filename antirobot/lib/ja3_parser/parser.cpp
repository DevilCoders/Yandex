#include "parser.h"

namespace Ja3Parser {
    namespace {
        const THashMap<TStringBuf, float TJa3::*> Factors = {
            {"159", &TJa3::C159},
            {"57-61", &TJa3::C57_61},
            {"159", &TJa3::C159},
            {"53", &TJa3::C53},
            {"60-49187", &TJa3::C60_49187},
            {"53-49187", &TJa3::C53_49187},
            {"52393-103", &TJa3::C52393_103},
            {"49162", &TJa3::C49162},
            {"50", &TJa3::C50},
            {"51", &TJa3::C51},
            {"255", &TJa3::C255},
            {"52392", &TJa3::C52392},
            {"10", &TJa3::C10},
            {"157-49200", &TJa3::C157_49200},
            {"49200", &TJa3::C49200},
            {"49171-103", &TJa3::C49171_103},
            {"49191-52394", &TJa3::C49191_52394},
            {"49192-52394", &TJa3::C49192_52394},
            {"65-52394", &TJa3::C65_52394},
            {"157", &TJa3::C157},
            {"52393-49200", &TJa3::C52393_49200},
            {"49159", &TJa3::C49159},
            {"4865", &TJa3::C4865},
            {"158-61", &TJa3::C158_61},
            {"49196-47", &TJa3::C49196_47},
            {"103", &TJa3::C103},
            {"103-49196", &TJa3::C103_49196},
            {"52393-49188", &TJa3::C52393_49188},
            {"60-65", &TJa3::C60_65},
            {"49195-69", &TJa3::C49195_69},
            {"154", &TJa3::C154},
            {"49187-49188", &TJa3::C49187_49188},
            {"49199-60", &TJa3::C49199_60},
            {"49195-51", &TJa3::C49195_51},
            {"49235", &TJa3::C49235},
            {"47", &TJa3::C47},
            {"49169", &TJa3::C49169},
            {"49249", &TJa3::C49249},
            {"49171-60", &TJa3::C49171_60},
            {"49188-49196", &TJa3::C49188_49196},
            {"61", &TJa3::C61},
            {"156-255", &TJa3::C156_255},
            {"47-57", &TJa3::C47_57},
            {"186", &TJa3::C186},
            {"49245", &TJa3::C49245},
            {"156-52394", &TJa3::C156_52394},
            {"20", &TJa3::C20},
            {"49188-49195", &TJa3::C49188_49195},
            {"52394-52392", &TJa3::C52394_52392},
            {"53-49162", &TJa3::C53_49162},
            {"49191", &TJa3::C49191},
            {"49245-49249", &TJa3::C49245_49249},
            {"49171", &TJa3::C49171},
            {"53-52393", &TJa3::C53_52393},
            {"51-49199", &TJa3::C51_49199},
            {"49234", &TJa3::C49234},
            {"49315", &TJa3::C49315},
            {"158", &TJa3::C158},
            {"49187-49161", &TJa3::C49187_49161},
            {"107", &TJa3::C107},
            {"52394", &TJa3::C52394},
            {"49162-61", &TJa3::C49162_61},
            {"153", &TJa3::C153},
            {"49170", &TJa3::C49170},
            {"156", &TJa3::C156},
            {"52393-60", &TJa3::C52393_60},
            {"49195-49192", &TJa3::C49195_49192},
            {"7", &TJa3::C7},
            {"49187-103", &TJa3::C49187_103},
            {"61-49172", &TJa3::C61_49172},
            {"159-49188", &TJa3::C159_49188},
            {"49171-49187", &TJa3::C49171_49187},
            {"49196-49188", &TJa3::C49196_49188},
            {"158-49161", &TJa3::C158_49161},
            {"49193", &TJa3::C49193},
        };
    }

    void TJa3::SetFactor(TStringBuf key, float value) {
        if (const auto it = Factors.find(key); it != Factors.end()) {
            this->*(it->second) = value;
        }
    }

    TJa3 StringToJa3(TStringBuf buf) {
        TJa3 ja3;
        TStringBuf token;

        // version
        token = buf.NextTok(',');
        ja3.TlsVersion = FromString(token);
        token = buf.NextTok(',');

        // ciphers
        TVector<TStringBuf> ciphers;
        float pos = 0;

        while (!token.empty()) {
            ciphers.push_back(token.NextTok('-'));
        }

        ja3.CiphersLen = static_cast<float>(ciphers.size());

        for (TStringBuf cipher : ciphers) {
            ja3.SetFactor(cipher, 1 / (1 + pos));
            ++pos;
        }

        for (size_t i = 1; i < ciphers.size(); ++i) {
            TString bigram = ToString(ciphers[i - 1]) + "-" + ToString(ciphers[i]);
            ja3.SetFactor(bigram, 1);
        }

        token = buf.NextTok(',');

        // extensions
        while (!token.empty()) {
            token.NextTok('-');
            ++ja3.ExtensionsLen;
        }

        return ja3;
    }
} // namespace Ja3Parser
