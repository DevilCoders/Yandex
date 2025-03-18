-- {{ file }} (file this query was created with)
$logs = "logs/awaps-log/1d";
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';
select
  fielddate, template_name,
  count_if(log.parameterstr like '%aadb=2%') as aab_shows,
  count(*) as total_shows
from range($logs, $table_from, $table_to) as log
join `home/antiadb/monitorings/adid_template` as templates on templates.adid = CAST(log.adid as UInt64)
where log.sectionid = '9001' and log.actionid = '0'
group by
  String::Substring(log.iso_eventtime, 0, 14) || '00:00' as fielddate,
  templates.template_name as template_name
