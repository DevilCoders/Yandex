{% import 'utils.tpl' as utils %}
{% import 'wiki_common.tpl' as wiki_common %}

{% macro metric_cell(metric) %}
%%(wacko wrapper=text align=right)
{% if metric.exp_result %}
{{ wiki_common.metric_entry(metric.color_by_value, 'experiment', utils.fmt(metric.exp_result.metric_values.significant_value)) }}
{% endif %}
{% if metric.control_result %}
{{ wiki_common.metric_entry(metric.color_by_value, 'control', utils.fmt(metric.control_result.metric_values.significant_value)) }}
{% endif %}
{% if metric.exp_result %}
{% for crit_res in metric.exp_result.criteria_results %}
{{ wiki_common.metric_entry(metric.color_by_value, crit_res.criteria_key.pretty_name().replace('[', '\\[') + ' p-value', utils.fmt(crit_res.pvalue)) }}
{% endfor %}
{% endif %}
{% if 'diff' in metric %}
{{ wiki_common.metric_entry(metric.color_by_value, 'diff', utils.fmt(metric.diff.significant.abs_diff)) }}
{{ wiki_common.metric_entry(metric.color_by_value, 'diff %', utils.fmt(metric.diff.significant.perc_diff_human, 2)) }}
{% if "verdict_discrepancy" in metric %}!!Расхождение с вердиктом!!{% endif %}
{% endif %}
%%
{% endmacro %}

{% if source_url %}(({{ source_url }} Operation URL)) {% endif %}
{{ wiki_common.error_box(validation_errors) }}

**Threshold** = {{threshold}}

#|
||
**Observation ID** / **Test ID** / **Control ID**
|**Name** / **Ticket**
|**Start date** / **End date** / **Days**
{% for metric in metrics.keys() %}
|**{{ metric.pretty_name().replace('[', '\\[') }}**
{%- endfor %}
||


{% for row in data_horizontal %}
{% if (show_all_gray or (not row.all_gray)) and (show_same_color or (not row.same_color)) %}
||
observation: ((https://ab.yandex-team.ru/observation/{{ row.observation.id }} {{ row.observation.id }}))
{% if row.observation.tags %}
tags: {{ row.observation.tags | join(", ") }}
{% endif %}
experiment: ((https://ab.yandex-team.ru/testid/{{ row.experiment.testid }} {{ row.experiment.testid }}))
{% if row.experiment.serpset_id %}(serpset {{ row.experiment.serpset_id }}){% endif %}

control: ((https://ab.yandex-team.ru/testid/{{ row.control.testid }} {{ row.control.testid }}))
{% if row.control.serpset_id %}(serpset {{ row.control.serpset_id }}){% endif %}

{% if row.discrepancy %}!!(red)(?Расхождение Часть метрик красная, а часть - зеленая?)!!{% endif %}
{% if row.split_changes %}!!(red)(?Переразбиение Даты переразбиений: {{ row.split_changes.keys() | sort | join(", ") }}?)!!{% endif %}
|""{{ row.abt_exp_title }}""
{{ row.ticket }}
|{{ row.observation.dates.start }} - {{ row.observation.dates.end }} ({{ row.observation.dates.number_of_days() }} days)
{% for metric in row.metrics %}
|
{% if metric != None %}
{{ metric_cell(metric) }}
{%- endif %}
{%- endfor %}
||
{% endif %}
{%- endfor %}


{% if stats %}
|| | |
{% for metric, _ in metrics | dictsort %}
|{{ wiki_common.metric_stats(stats[metric]) }}
{%- endfor %}
||
{% endif %}

|#
