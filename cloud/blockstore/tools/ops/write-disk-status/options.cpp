#include "options.h"

#include <util/generic/serialized_enum.h>

#include <library/cpp/getopt/small/last_getopt.h>

using namespace NLastGetopt;
using namespace NCloud::NBlockStore;

////////////////////////////////////////////////////////////////////////////////

namespace {

    struct TProfile
    {
        TString Endpoint;
        TString Database;
        TString IamEndpoint;
        TString AccountId;
    };

    // Endpoint & Database: see logbroker documentation (https://nda.ya.ru/t/eZY_YaAA3mvev4)
    const THashMap<TString, TProfile> Profiles = {
        { "prod", TProfile {
            .Endpoint = "lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net",
            .Database = "/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla",
            .IamEndpoint = "iam.api.cloud.yandex.net",
            .AccountId = "b1grjf2o6a6f1fmqeu6j"
        }},
        { "preprod", TProfile {
            .Endpoint = "lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net",
            .Database = "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3",
            .IamEndpoint = "iam.api.cloud-preprod.yandex.net",
            .AccountId = "aoeeuk6nmfvdsn75kksu"
        }},
        { "testing", TProfile {
            .Endpoint = "lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net",
            .Database = "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3",
            .IamEndpoint = "iam.api.cloud-preprod.yandex.net",
            .AccountId = "aoeeuk6nmfvdsn75kksu"
        }},
        { "dev", TProfile {
            .Endpoint = "lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net",
            .Database = "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3",
            .IamEndpoint = "iam.api.cloud-preprod.yandex.net",
            .AccountId = "aoeeuk6nmfvdsn75kksu"
        }}
    };

    TString CreateTopicPath(const TString& accountId, const TString& profileName)
    {
        return "/" + accountId + "/" + profileName + "/disk-state";
    }

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TOptions::Parse(int argc, char** argv)
{
    TOpts opts;
    opts.AddHelpOption();

    opts.AddLongOption("disk-id", "disk id")
        .RequiredArgument("STR")
        .Required()
        .StoreResult(&DiskId);

    opts.AddLongOption("disk-state", "disk state")
        .RequiredArgument("{" + GetEnumAllNames<EDiskState>() + "}")
        .DefaultValue(EDiskState::Online)
        .Handler1T<TString>([this] (const auto& s) {
            DiskState = FromString<EDiskState>(s);
        });

    opts.AddLongOption("message", "status message")
        .RequiredArgument("STR")
        .StoreResult(&Message);

    opts.AddLongOption("lb-topic", "topic")
        .RequiredArgument("STR")
        .StoreResult(&Topic);

    opts.AddLongOption("lb-source-id", "source id")
        .RequiredArgument("STR")
        .StoreResult(&SourceId);

    TString profileName;
    opts.AddLongOption("profile", "profile name (prod, preprod)")
        .RequiredArgument("STR")
        .StoreResult(&profileName);

    opts.AddLongOption("lb-endpoint", "logbroker endpoint")
        .RequiredArgument("STR")
        .StoreResult(&Endpoint);

    opts.AddLongOption("lb-database", "logbroker database")
        .RequiredArgument("STR")
        .StoreResult(&Database);

    opts.AddLongOption("lb-reconnect-on-failure", "auto reconnect on failure")
        .NoArgument()
        .StoreTrue(&ReconnectOnFailure);

    opts.AddLongOption("lb-timeout", "timeout")
        .RequiredArgument("DURATION")
        .DefaultValue(TDuration::Minutes(1))
        .StoreResult(&Timeout);

    opts.AddLongOption("lb-seq-no", "sequence number")
        .RequiredArgument("NUM")
        .DefaultValue(SeqNo)
        .StoreResult(&SeqNo);

    opts.AddLongOption("verbose", "output level for diagnostics messages")
        .OptionalArgument("{" + GetEnumAllNames<ELogLevel>() + "}")
        .Handler1T<TString>([this] (const auto& s) {
            VerboseLevel = s
                ? FromString<ELogLevel>(s)
                : ELogLevel::Debug;
        });

    opts.AddLongOption("lb-use-oauth", "authorization via OAuth (LB_OAUTH_TOKEN env. variable)")
        .NoArgument()
        .StoreTrue(&UseOAuth);

    opts.AddLongOption("lb-iam-jwt-file", "authorization via JSON Web Token")
        .RequiredArgument("FILE")
        .StoreResult(&IamJwtKeyFilename);

    opts.AddLongOption("iam-endpoint", "IAM endpoint")
        .RequiredArgument("STR")
        .StoreResult(&IamEndpoint);

    opts.AddLongOption("ca-cert-file", "")
        .RequiredArgument("STR")
        .DefaultValue("")
        .StoreResult(&CaCertFilename);

    TOptsParseResultException res(&opts, argc, argv);
    Y_UNUSED(res);

    if (profileName) {
        const auto& profile = Profiles.at(profileName);

        if (!Endpoint) {
            Endpoint = profile.Endpoint;
        }

        if (!Database) {
            Database = profile.Database;
        }

        if (!IamEndpoint) {
            IamEndpoint = profile.IamEndpoint;
        }

        if (!Topic) {
            Topic = CreateTopicPath(profile.AccountId, profileName);
        }
    }

    Y_ENSURE(Topic, "Topic is required");
}
