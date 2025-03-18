#include "database.h"

#include <antirobot/lib/addr.h>

#include <library/cpp/resource/resource.h>

#include <maps/libs/pgpool/include/pgpool3.h>
#include <maps/libs/pgpool/include/pool_configuration.h>

#include <pqxx/pqxx>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/hex.h>
#include <util/string/vector.h>

#include <tuple>


namespace NCbbFast {


namespace {


TString MakeGetRangeLineQuery(TStringBuf type) {
    return TStringBuilder() <<
        "SELECT "
            "cbb.cbb_range_" << type << ".created, "
            "cbb.cbb_range_" << type << ".expire, "
            "cbb.cbb_range_" << type << ".id, "
            "cbb.cbb_range_" << type << ".rng_" << type << " AS line "
        "FROM "
            "cbb.cbb_range_" << type << " "
        "WHERE "
            "cbb.cbb_range_" << type << ".group_id = ";
}

TString FormatTime(const TInstant& time) {
    TStringStream out;
    out << "'"
        << time.FormatGmTime("%Y-%m-%d %H:%M:%S")
        << "." << time.MicroSecondsOfSecond()
        << "'";
    return out.Str();
}

const TString GET_RANGE_RE_QUERY = MakeGetRangeLineQuery("re");
const TString GET_RANGE_TXT_QUERY = MakeGetRangeLineQuery("txt");


constexpr TDuration TimeOffset = TDuration::Hours(3);


NAntiRobot::TAddr ParseIp6(const char* ipStr) {
    TStringBuf buf(ipStr);
    TStringBuf bufWithoutPrefix;
    buf.AfterPrefix("\\x", bufWithoutPrefix);
    const TString ipDecoded = HexDecode(bufWithoutPrefix);
    return NAntiRobot::TAddr::FromIp6(ipDecoded);
}


TGroup::EType ParseGroupType(TStringBuf s) {
    if (EqualToOneOf(s, "0", "1", "2")) {
        return TGroup::EType::Ip;
    } else if (s == "3") {
        return TGroup::EType::Txt;
    } else if (s == "4") {
        return TGroup::EType::Re;
    } else {
        ythrow yexception() << "Invalid group type: " << s;
    }

}


} // anonymous namespace


TBaseRule::TBaseRule(const pqxx::row& row, bool hasId) {
    if (hasId) {
        Id = row.at("id").as<ui32>();
    }

    if (const auto expire = row.at("expire"); !expire.is_null()) {
        Expired = TInstant::ParseIso8601(expire.c_str());
    }

    if (const auto created = row.at("created"); !created.is_null()) {
        Created = TInstant::ParseIso8601(created.c_str());
    }
}


TLineRule::TLineRule(const pqxx::row& row)
    : TBaseRule(row)
{
    RangeLine = row.at("line").c_str();
}


TIpRule TIpRule::FromIpv4(const pqxx::row& row) {
    return TIpRule(
        row,
        NAntiRobot::TAddr::FromIp(row.at("ipv4_rng_start").as<ui32>()),
        NAntiRobot::TAddr::FromIp(row.at("ipv4_rng_end").as<ui32>())
    );
}

TIpRule TIpRule::FromIpv6(const pqxx::row& row) {
    return TIpRule(
        row,
        ParseIp6(row.at("ipv6_rng_start").c_str()),
        ParseIp6(row.at("ipv6_rng_end").c_str())
    );
}

TIpRule::TIpRule(const pqxx::row& row, NAntiRobot::TAddr start, NAntiRobot::TAddr end)
    : TBaseRule(row, false)
    , RangeStart(std::move(start))
    , RangeEnd(std::move(end))
    , Serialized(RangeStart.ToString())
{}


TDatabase::TDatabase(const TConfig& config) {
    {
        TFileOutput cert(CertFile.Name());
        cert << NResource::Find("root.crt");
    }

    TVector<maps::pgpool3::InstanceId> hosts;
    for (size_t i = 0; i < config.DbHostsSize(); ++i) {
        hosts.emplace_back(config.GetDbHosts(i), config.GetDbPort());
    }

    maps::pgpool3::PoolConfiguration configuration(hosts);

    TStringStream connParamsStream;
    connParamsStream
        << "dbname=" << config.GetDbName()
        << " user=" << config.GetDbUser()
        << " password=" << config.GetDbPassword();

    if (!config.GetDebug()) {
        connParamsStream
            << " sslmode=" << config.GetDbSslMode()
            << " sslrootcert=" << CertFile.Name();
    }

    std::string connParams = connParamsStream.Str();

    auto constants = maps::pgpool3::PoolConstants(
        10, // master connection count
        50, // max master connection count
        10, // slave connection count
        50  // max slave connection count
    );

    // by default treatMasterAsSlave = false;
    // and we use slaveTransaction
    // master autodetection is on

    Pool = new maps::pgpool3::Pool(
        std::move(configuration),
        connParams,
        constants
    );
}

TMaybe<TGroup> TDatabase::CheckFlag(ui32 groupId) {
    auto txn = Pool->slaveTransaction();
    TStringBuilder query;

    query <<
        "SELECT "
            "cbb.cbb_groups.updated AS updated, "
            "cbb.cbb_groups.default_type AS default_type "
        "FROM "
            "cbb.cbb_groups "
        "WHERE cbb.cbb_groups.id = " << groupId;

    pqxx::result result;

    try {
        result = txn->exec(query);
        txn->commit();
    } catch (const std::exception& e) {
        Cerr << "CheckFlag: " << e.what() << Endl;
    }

    if (result.empty()) {
        return Nothing();
    }

    return TGroup(
        ParseGroupType(result.front().at("default_type").c_str()),
        TInstant::ParseIso8601(result.front().at("updated").c_str()) - TimeOffset,
        std::monostate()
    );
}

TUpdatedTimes TDatabase::GetUpdated() {
    auto txn = Pool->slaveTransaction();

    const TString query = 
        "SELECT "
            "cbb.cbb_groups.updated AS updated, "
            "cbb.cbb_groups.id AS id "
        "FROM "
            "cbb.cbb_groups";

    pqxx::result result;

    try {
        result = txn->exec(query);
        txn->commit();
    } catch (const std::exception& e) {
        Cerr << "GetUpdated: " << e.what() << Endl;
    }

    if (result.empty()) {
        return {};
    }

    TUpdatedTimes updatedTimes;

    for (const auto it : result) {
        if (const TStringBuf updatedStr = it.at("updated").c_str(); !updatedStr.empty()) {
            if (TInstant updated; TInstant::TryParseIso8601(updatedStr, updated)) {
                updatedTimes[FromString<ui32>(it.at("id").c_str())] = updated - TimeOffset;
            }
        }
    }

    return updatedTimes;
}

bool TDatabase::GetRange(ui32 groupId, TGroup* group) {
    TGroup::TRules rules;

    switch (group->Type) {
    case TGroup::EType::Re:
        rules = GetRangeLine(GET_RANGE_RE_QUERY, groupId);
        break;

    case TGroup::EType::Txt:
        rules = GetRangeLine(GET_RANGE_TXT_QUERY, groupId);
        break;

    case TGroup::EType::Ip:
        rules = GetRangeIp(groupId);
        break;
    }

    if (std::get_if<std::monostate>(&rules)) {
        return false;
    }

    group->Rules = std::move(rules);

    return true;
}

bool TDatabase::AddIps(const TAddIpsParams& params) {
    using NAntiRobot::TAddr;

    TVector<TAddr> addrs;
    for (const auto& ip : params.Addrs) {
        NAntiRobot::TAddr addr(ip);
        addrs.push_back(addr);
    }

    TVector<TAddr> ipv4Addrs;
    TVector<TAddr> ipv6Addrs;

    for (const auto& ip : addrs) {
        if (ip.IsIp4()) {
            ipv4Addrs.push_back(ip);
        } else if (ip.IsIp6()) {
            ipv6Addrs.push_back(ip);
        }
    }

    TInstant created = TInstant::Now() + TimeOffset;
    TInstant expire = created + params.Timeout;
    const auto now = TInstant::Now().ToString();


    TStringBuilder query;
    std::array<std::tuple<
        TVector<TAddr>,
        TString,
        std::function<TString(const TAddr&)>
        >, 2> ipData = {{
            {ipv4Addrs, "cbb_range_ipv4", [](const TAddr& ip) { return ToString(ip.AsIp()); }},
            {ipv6Addrs, "cbb_range_ipv6", [](const TAddr& ip) { return TString("'\\x") + HexEncode(ip.AsIp6()) + "'"; }},
        }};
    for (auto& [ipAddrs, table, dumpIp] : ipData) {
        if (!ipAddrs.empty()) {
            size_t currentCount = 0;
            {
                TStringBuilder countQuery;
                countQuery << "SELECT COUNT(*) as count"
                    "     FROM cbb." << table << " WHERE" 
                    "       cbb." << table << ".expire IS NULL "
                    "       OR cbb." << table << ".expire >= '" << now << "' "
                    "       AND cbb." << table << ".group_id = " << params.Flag << ";";

                auto txn = Pool->slaveTransaction();
                pqxx::result rows;

                try {
                    rows = txn->exec(countQuery);
                    txn->commit();
                } catch (std::exception& e) {
                    Cerr << "GetCount: " << e.what() << Endl;
                    return false;
                }
                if (rows.size()) {
                    currentCount = rows.front().at("count").as<ui32>();
                }

            }
            ipAddrs.resize(Min(ipAddrs.size(), params.MaxIps - currentCount));
            if (ipAddrs.empty()) {
                continue;
            }

            query << " INSERT INTO "
                            "cbb." << table << " "
                            "(group_id, created, block_descr, expire, \"user\", is_exception, block_type, rng_start, rng_end) "
                        " VALUES ";

            size_t numIps = ipAddrs.size();
            for (const auto& ip : ipAddrs) {
                query 
                    << "("
                    << params.Flag << ", "          // group_id
                    << FormatTime(created) << ", "  // created
                    << "''" << ", "                 // block_descr
                    << FormatTime(expire) << ", "   // expire
                    << "'" << params.User << "', "  // user
                    << "False" << ", "              // is_exception
                    << "'single_ip'" << ", "        // block_type
                    << dumpIp(ip) << ", "           // rng_start
                    << dumpIp(ip)                   // eng_end
                    << ")";

                if (--numIps) {
                    query << ", ";
                }
            }
            query << " ON CONFLICT"
                        " (group_id, is_exception, rng_start, rng_end)"
                        " DO UPDATE"
                        " SET created = EXCLUDED.created, expire = EXCLUDED.expire; ";
        }
    }

    if (query.empty()) {
        return false;
    }

    query << "UPDATE cbb.cbb_groups"
        << " SET updated = " << FormatTime(created)
        << " WHERE id = " << params.Flag << "; ";

    auto txn = Pool->masterWriteableTransaction();

    pqxx::result rows;
    rows = txn->exec(TStringBuilder() << query);
    txn->commit();

    return true;
}

TGroup::TRules TDatabase::GetRangeLine(TStringBuf query, ui32 groupId) {
    auto txn = Pool->slaveTransaction();

    pqxx::result rows;

    try {
        rows = txn->exec(TStringBuilder() << query << groupId);
        txn->commit();
    } catch (const std::exception& e) {
        Cerr << "GetRangeLine: " << e.what() << Endl;
        return std::monostate();
    }

    TVector<TLineRule> rules;
    rules.reserve(rows.size());

    for (const auto& row : rows) {
        rules.push_back(TLineRule(row));
    }

    return rules;
}

TGroup::TRules TDatabase::GetRangeIp(ui32 groupId) {
    static const std::array<std::tuple<
        TString,
        TString,
        TIpRule(*)(const pqxx::row&)>
    , 2> versions = {{
        {"cbb_range_ipv4", "ipv4", TIpRule::FromIpv4},
        {"cbb_range_ipv6", "ipv6", TIpRule::FromIpv6},
    }};

    const auto expire = TInstant::Now().ToString();

    TVector<TIpRule> rules;

    for (const auto& [table, prefix, ctor] : versions) {
        TStringBuilder query;
        query << "SELECT "
            << "cbb." << table << ".created AS created, "
            << "cbb." << table << ".expire AS expire, "
            << "cbb." << table << ".rng_start AS " << prefix << "_rng_start, "
            << "cbb." << table << ".rng_end AS " << prefix << "_rng_end "
            << "FROM "
            << "cbb." << table << " "
            << "WHERE ( "
            << "cbb." << table << ".expire IS NULL "
            << "OR cbb." << table << ".expire >= '" << expire << "') "
            << "AND cbb." << table << ".group_id = " << groupId;

        auto txn = Pool->slaveTransaction();

        pqxx::result rows;

        try {
            rows = txn->exec(query);
            txn->commit();
        } catch (std::exception& e) {
            Cerr << "GetRangeIp: " << e.what() << Endl;
            return std::monostate();
        }

        rules.reserve(rules.size() + rows.size());

        for (const auto& row : rows) {
            rules.push_back(ctor(row));
        }
    }

    return rules;
}


} // namespace NCbbFast
