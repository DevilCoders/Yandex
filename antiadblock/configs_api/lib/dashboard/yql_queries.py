WEEK_RULES_QUERY = r'''$dfa = ($added) -> {return DateTime::ToDays(DateTime::IntervalFromSeconds(
DateTime::ToSeconds(CurrentUtcTimestamp()) - DateTime::ToSeconds(DateTime::MakeDatetime(DateTime::ParseIso8601($added)))));};
$re = Pire::Grep('\\+js\\(cookie-remover(\\.js)?,|scriptlet\\(remove-cookie,|AG_removeCookie\\(|\\$cookie=');
$filtered_rules = select added, partner from hahn.`home/antiadb/sonar/sonar_rules` where not $re(raw_rule);
$today_rules = (
    select
        count(*) as rules_count, partner
    from (
        select
            $dfa(added) as days_from_added,
            partner
            from $filtered_rules
            where $dfa(added) > -1 and $dfa(added) < 8
        )
    GROUP BY partner
);
$yesterday_rules = (
    select
        count(*) as rules_count, partner
    from (
        select
            $dfa(added) as days_from_added,
            partner
            from $filtered_rules
            where $dfa(added) > 0 and $dfa(added) < 8
        )
    GROUP BY partner
);
$rules_count_table = (
select COALESCE(t.rules_count, 0) as t_count, COALESCE(y.rules_count, 0) as y_count, t.partner as service_id
from $today_rules as t full join $yesterday_rules as y on t.partner = y.partner);

$result = select service_id, "yesterday: "|| CAST(y_count as String) || "\ntoday: " || CAST(t_count as String) as value, if(t_count - y_count > 0, "red", "green") as status
from $rules_count_table;

$all_services = select * from (select AsList(__ANTIADB_SERVICE_IDS__) as service_id) flatten by service_id;

select all_services.service_id, COALESCE(value, 'yesterday: 0\ntoday: 0') as value, COALESCE(status, 'green')
 from $result as result right join $all_services as all_services on result.service_id = all_services.service_id;'''


PCODE_VERSIONS_QUERY = '''
$all_services = select * from (select AsList(__ANTIADB_SERVICE_IDS__) as service_id) flatten by service_id;
select all_services.service_id, description, status from `home/antiadb/dashboard/pcode_versions` as result
 right join $all_services as all_services on result.service_id = all_services.service_id where version_position = {_version_position_};
'''
