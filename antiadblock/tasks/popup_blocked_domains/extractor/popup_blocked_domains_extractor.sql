pragma RegexUseRe2='true';

$capture = Re2::Capture(@@^(?:[|]{2})?([^:\/?$&^|]+\.[^:\/?$&^|]+)(?:[\/^?].*)?$@@);

$query = (
select distinct
    $capture(value)._1 as domain,
    raw_rule,
    list_url
from
    `home/antiadb/sonar/general_rules`
where
    action='block' and type='url-pattern' and options like '%popup%'
);

{% if upload_to_yt %}
insert into `{{ yt_path }}` with truncate
{% endif %}
select * from $query where domain is not NULL;
{% if upload_to_yt %}
commit;
{% endif %}
