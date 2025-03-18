pragma RegexUseRe2='true';

$capture = Re2::Capture(@@^(?:[|]{2})?([^:\/?$&^|]+\.[^:\/?$&^|]+)(?:[\/^?].*)?$@@);

$query = (select
    $capture(value)._1 as domain,
    list_url, value, raw_rule, type, is_yandex_related
from
    `home/antiadb/sonar/general_rules`
where
    action='block' and type='url-pattern' and options like '%popup%'
    and not (value REGEXP @@^[&=?].*$@@) /* exclude things like &param=value... or =direct&... or ?param */
    and not (value REGEXP @@^\/[a-zA-Z0-9?&*].*$@@)  /* exclude rules like /path and /?query */
    and (value REGEXP @@^.*[.*_].*$@@) /* exclude rules that doesn't contain any dots or stars or _ */
    and not (value REGEXP @@^([|]{2})?\.[0-9a-zA-Z-]+[?\/].*$@@) /* exclude rules that look like ||.com/adasd */
    and not (value REGEXP @@^[|]{2}[0-9a-zA-Z-]+[?\/].*$@@) /* exclude rules that look like ||asdcom/adasd */
    and not (value like '|javascript:%')  /* exclude rules like |javascript: */
);

select distinct * from $query where domain is NULL;
