#include <antirobot/lib/keyring.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/lib/spravka_key.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/hex.h>

int main(int argc, const char* argv[])
{
    using namespace NLastGetopt;

    TString keys, key, domain, dataKey;

    TOpts opts;
    opts.AddLongOption("keys", "file name with keys to decode spravka").StoreResult(&keys).OptionalArgument("file name");
    opts.AddLongOption("key", "key to decode spravka").StoreResult(&key).OptionalArgument("key string");
    opts.AddLongOption("domain", "domain").StoreResult(&domain).OptionalArgument("domain string");
    opts.AddLongOption("data-key", "key to decode spravka data").StoreResult(&dataKey).RequiredArgument("secret key");


    opts.SetFreeArgsMin(1);
    opts.SetFreeArgsMax(1);

    TOptsParseResult optRes(&opts, argc, argv);

    TStringInput dataKeySi(dataKey);
    NAntiRobot::TSpravkaKey::SetInstance(NAntiRobot::TSpravkaKey(dataKeySi));

    NAntiRobot::TSpravka spravka;

    if (!keys.empty()) {
        TUnbufferedFileInput inp(keys);
        NAntiRobot::TKeyRing::SetInstance(NAntiRobot::TKeyRing(inp));
    } else if (!key.empty()) {
        TStringInput inp(key);
        NAntiRobot::TKeyRing::SetInstance(NAntiRobot::TKeyRing(inp));
    } else {
        ythrow yexception() << "Keys file or key must be specified";
    }

    const auto freeArgs = optRes.GetFreeArgs();
    TStringBuf spravkaStr(freeArgs[0]);

    if (spravka.Parse(spravkaStr, domain)) {
        Cerr << "OK";
        return 0;
    }

    Cerr << "BAD";
    return 1;
}
