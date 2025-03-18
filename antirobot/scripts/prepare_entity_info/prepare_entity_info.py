import json
import nirvana.mr_job_context as nv
import yt.wrapper as yt
from yql.api.v1.client import YqlClient

REQS_QUERY_API_TEMPLATE = '''
SELECT
    req_url,
    count(*) as c,
    count_if(jws_state == 'VALID') as c_valid,
    1. * count_if(jws_state == 'VALID') / count(*) as c_valid_ratio
FROM
    `{source}`
WHERE
    {degradation}
    (ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103433') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103434') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103435')) AND
    ja3 == '{ja3}' AND
    service = '{service}' AND
    ip_list == ''
GROUP BY
    req_url
ORDER BY
    c DESC'''

REQS_QUERY_TEMPLATE = '''SELECT
    req_url,
    count(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    ja3 == '{ja3}' AND
    service = '{service}' AND
    ip_list == ''
GROUP BY
    req_url
ORDER BY
    c DESC'''

SUBNETS_QUERY_API_TEMPLATE = '''
SELECT
    subnet,
    Geo::IsHosting(subnet) as is_hosting,
    Geo::IsProxy(subnet) as is_proxy,
    Geo::IsTor(subnet) as is_tor,
    Geo::IsVpn(subnet) as is_vpn,
    Geo::IsMobile(subnet) as is_mobile,
    Geo::IsYandex(subnet) as is_yandex,
    Geo::IsYandexStaff(subnet) as is_yandex_staff,
    Geo::IsYandexTurbo(subnet) as is_yandex_turbo,
    Geo::RegionByIp(subnet).en_name as region,
    Geo::GetIspNameByIp(subnet) as isp_name,
    Geo::GetOrgNameByIp(subnet) as org_name,
    COUNT(*) as c,
    count_if(jws_state == 'VALID') as c_valid,
    1. * count_if(jws_state == 'VALID') / count(*) as c_valid_ratio
FROM
    `{source}`
WHERE
    {degradation}
    (ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103433') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103434') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103435')) AND
    ja3 == '{ja3}' AND
    service = '{service}' AND
    ip_list == ''
GROUP BY
    Ip::ToString(Ip::GetSubnet(Ip::FromString(user_ip))) as subnet
ORDER BY
    c DESC'''

SUBNETS_QUERY_TEMPLATE = '''
SELECT
    subnet,
    Geo::IsHosting(subnet) as is_hosting,
    Geo::IsProxy(subnet) as is_proxy,
    Geo::IsTor(subnet) as is_tor,
    Geo::IsVpn(subnet) as is_vpn,
    Geo::IsMobile(subnet) as is_mobile,
    Geo::IsYandex(subnet) as is_yandex,
    Geo::IsYandexStaff(subnet) as is_yandex_staff,
    Geo::IsYandexTurbo(subnet) as is_yandex_turbo,
    Geo::RegionByIp(subnet).en_name as region,
    Geo::GetIspNameByIp(subnet) as isp_name,
    Geo::GetOrgNameByIp(subnet) as org_name,
    COUNT(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    ja3 == '{ja3}' AND
    service = '{service}' AND
    ip_list == ''
GROUP BY
    Ip::ToString(Ip::GetSubnet(Ip::FromString(user_ip))) as subnet
ORDER BY
    c DESC'''

UAS_QUERY_API_TEMPLATE = '''
SELECT
    user_agent,
    count(*) as c,
    count_if(jws_state == 'VALID') as c_valid,
    1. * count_if(jws_state == 'VALID') / count(*) as c_valid_ratio
FROM
    `{source}`
WHERE
    {degradation}
    (ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103433') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103434') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103435')) AND
    ja3 == '{ja3}' AND
    service = '{service}' AND
    ip_list == ''
GROUP BY
    user_agent
ORDER BY
    c DESC'''

UAS_QUERY_TEMPLATE = '''
SELECT
    user_agent,
    count(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    ja3 == '{ja3}' AND
    service = '{service}' AND
    ip_list == ''
GROUP BY
    user_agent
ORDER BY
    c DESC'''

JA3_DICT_TEMPLATE = '''
SELECT
    `service`,
    `cnt_events`,
    `share_on_service`,
    `top_50_ua`,
    `trust_share`,
    `login_share`,
    `yandex_share`,
    `hosting_share`,
    `foreign_share`,
    `dist_yuids`,
    `first_time`,
    `last_time`,
    `ja3`
FROM
    `//home/antirobot/export/ja3stats/all_stat`
WHERE
    ja3 == '{ja3}'
ORDER BY
    cnt_events DESC
LIMIT 100;
'''

ST_TEMPLATE = '''<{{{idx}. {ja3}

  Поиск по ЕББ: ((https://cbb-ext.yandex-team.ru/search?search={ja3} https://cbb-ext.yandex-team.ru/search?search={ja3}))
  Топ ручек: {req_urls_url}
  Топ подсетей: {subnets_url}
  Топ user_agent'ов: {uas_url}
  Инфа по словарю: {ja3s_url}

}}>
----'''

SUBNET_REQS_QUERY_API_TEMPLATE = '''
$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

SELECT
    req_url,
    count(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    (ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103433') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103434') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103435')) AND
    $subnet(user_ip) == '{subnet}' AND
    service = '{service}'
GROUP BY
    req_url
ORDER BY
    c DESC'''

SUBNET_REQS_QUERY_TEMPLATE = '''
$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

SELECT
    req_url,
    count(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    $subnet(user_ip) == '{subnet}' AND
    service = '{service}'
GROUP BY
    req_url
ORDER BY
    c DESC'''

SUBNET_JA3S_QUERY_API_TEMPLATE = '''
$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

SELECT
    ja3,
    COUNT(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    (ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103433') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103434') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103435')) AND
    $subnet(user_ip) == '{subnet}' AND
    service = '{service}'
GROUP BY
    ja3
ORDER BY
    c DESC'''

SUBNET_JA3S_QUERY_TEMPLATE = '''
$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

SELECT
    ja3,
    COUNT(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    $subnet(user_ip) == '{subnet}' AND
    service = '{service}'
GROUP BY
    ja3
ORDER BY
    c DESC'''

SUBNET_UAS_QUERY_API_TEMPLATE = '''
$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

SELECT
    user_agent,
    count(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    (ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103433') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103434') OR
    ListHas(Yson::ConvertToStringList(cbb_mark_rules), '702#103435')) AND
    $subnet(user_ip) == '{subnet}' AND
    service = '{service}'
GROUP BY
    user_agent
ORDER BY
    c DESC'''

SUBNET_UAS_QUERY_TEMPLATE = '''
$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

SELECT
    user_agent,
    count(*) as c
FROM
    `{source}`
WHERE
    {degradation}
    $subnet(user_ip) == '{subnet}' AND
    service = '{service}'
GROUP BY
    user_agent
ORDER BY
    c DESC'''

SUBNET_DICT_TEMPLATE = '''
$ips = AsList(
    '{subnet}'
);

$subnet = ($ip) -> {{ RETURN Ip::ToString(Ip::GetSubnet(Ip::FromString($ip))); }};

$ip_data = ListMap($ips, ($x) -> {{ RETURN <|'ip': $x|>}});

SELECT
    ip,
    Geo::IsHosting(ip) as is_hosting,
    Geo::IsProxy(ip) as is_proxy,
    Geo::IsTor(ip) as is_tor,
    Geo::IsVpn(ip) as is_vpn,
    Geo::IsMobile(ip) as is_mobile,
    Geo::IsYandex(ip) as is_yandex,
    Geo::IsYandexStaff(ip) as is_yandex_staff,
    Geo::IsYandexTurbo(ip) as is_yandex_turbo,
    Geo::RegionByIp(ip).en_name as region,
    Geo::GetIspNameByIp(ip) as isp_name,
    Geo::GetOrgNameByIp(ip) as org_name
FROM
    AS_TABLE($ip_data);
'''

SUBNET_ST_TEMPLATE = '''<{{{idx}. {subnet}

  Поиск по ЕББ: https://cbb-ext.yandex-team.ru/search?search={subnet}
  Топ ручек: {req_urls_url}
  Топ ja3: {ja3s_url}
  Топ user_agent'ов: {uas_url}
  Инфа про подсеть: {subnet_url}

}}>
----'''


def run_query(client, query, title, **params):
    req = client.query(query.format(**params), syntax_version=1, title=title)
    req.run()
    return req.share_url


def check_ja3(ja3, service, source, db, degradation=None):
    client = YqlClient(db=db)
    params = {
        'service': service,
        'source': source,
        'ja3': ja3
    }
    if degradation is None:
        params['degradation'] = ''
    elif degradation:
        params['degradation'] = 'degradation AND'
    elif not degradation:
        params['degradation'] = 'NOT degradation AND'
    req_urls_url = run_query(client, REQS_QUERY_API_TEMPLATE if service == 'apiauto' else REQS_QUERY_TEMPLATE,
                             '[YQL] Top req_urls', **params)
    subnets_url = run_query(client, SUBNETS_QUERY_API_TEMPLATE if service == 'apiauto' else SUBNETS_QUERY_TEMPLATE,
                            '[YQL] Top subnets', **params)
    uas_url = run_query(client, UAS_QUERY_API_TEMPLATE if service == 'apiauto' else UAS_QUERY_TEMPLATE,
                        '[YQL] Top user_agents', **params)
    ja3s_url = run_query(client, JA3_DICT_TEMPLATE, '[YQL] Ja3 stats', **params)
    return (req_urls_url, subnets_url, uas_url, ja3s_url)


def print_ja3_desc(ja3, service, source, degradation, idx, db):
    req_urls_url, subnets_url, uas_url, ja3s_url = check_ja3(ja3, service, source, db, degradation)
    return ST_TEMPLATE.format(idx=idx, ja3=ja3, req_urls_url=req_urls_url, subnets_url=subnets_url, uas_url=uas_url,
                              ja3s_url=ja3s_url)


def check_subnet(subnet, service, source, db, degradation=None):
    client = YqlClient(db=db)
    params = {
        'service': service,
        'source': source,
        'subnet': subnet
    }
    if degradation is None:
        params['degradation'] = ''
    elif degradation:
        params['degradation'] = 'degradation AND'
    elif not degradation:
        params['degradation'] = 'NOT degradation AND'
    req_urls_url = run_query(client,
                             SUBNET_REQS_QUERY_API_TEMPLATE if service == 'apiauto' else SUBNET_REQS_QUERY_TEMPLATE,
                             '[YQL] Top req_urls', **params)
    ja3s_url = run_query(client, SUBNET_JA3S_QUERY_API_TEMPLATE if service == 'apiauto' else SUBNET_JA3S_QUERY_TEMPLATE,
                         '[YQL] Top ja3s', **params)
    uas_url = run_query(client, SUBNET_UAS_QUERY_API_TEMPLATE if service == 'apiauto' else SUBNET_UAS_QUERY_TEMPLATE,
                        '[YQL] Top user_agents', **params)
    subnet_url = run_query(client, SUBNET_DICT_TEMPLATE, '[YQL] Subnet info', **params)
    return (req_urls_url, ja3s_url, uas_url, subnet_url)


def print_subnet_desc(subnet, service, source, degradation, idx, db):
    req_urls_url, ja3s_url, uas_url, subnet_url = check_subnet(subnet, service, source, db, degradation)
    return SUBNET_ST_TEMPLATE.format(idx=idx, subnet=subnet, req_urls_url=req_urls_url, ja3s_url=ja3s_url,
                                     uas_url=uas_url, subnet_url=subnet_url)


def main():
    job_context = nv.context()
    inputs = job_context.get_mr_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()

    input_table = inputs.get('input').get_path()
    source = inputs.get('source').get_path()
    comment_output = outputs.get('comment')
    db = parameters.get('mr-default-cluster')
    title = parameters.get('title')
    limit = parameters.get('limit')
    entity = 'Ja3' if 'ja3' in parameters.get('entity') else 'подсети'

    service = parameters.get('service')

    degradation = False
    PREFIX = '''===={title}
    Подозрительные {entity}:
    '''.format(title=title, entity=entity)
    entities = list(map(lambda x: x['entity'], yt.read_table(input_table + '{{"entity"}}[:#{limit}]'.format(limit=limit))))

    comment = '\n'.join(
        [PREFIX] + [print_ja3_desc(e, service, source, degradation, idx + 1, db) for idx, e in enumerate(entities)])

    with open(comment_output, 'w') as f:
        json.dump({'comment': comment}, f)


if __name__ == "__main__":
    main()
