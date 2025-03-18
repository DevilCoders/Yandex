{% import 'utils.tpl' as utils %}
{% import 'wiki_common.tpl' as wiki_common %}

{{ wiki_common.error_box(validation_errors) }}

{% if source_url %}(({{ source_url }} Operation URL)) {% endif %}

**Threshold** = {{threshold}}

{% if stats %}
#|
||
**Metric**
|{% call wiki_common.colorize('green') %}**Green**{% endcall %}
|{% call wiki_common.colorize('yellow') %}**Yellow**{% endcall %}
|{% call wiki_common.colorize('red') %}**Red**{% endcall %}
|{% call wiki_common.colorize('gray') %}**Gray**{% endcall %}
|**Total**
||
{% for metric, _ in stats | dictsort %}
||
**{{ metric.pretty_name().replace('[', '\\[') }}**
|{% call wiki_common.colorize('green') %}{{ stats[metric].green }}{% endcall %}
|{% call wiki_common.colorize('yellow') %}{{ stats[metric].yellow }}{% endcall %}
|{% call wiki_common.colorize('red') %}{{ stats[metric].red }}{% endcall %}
|{% call wiki_common.colorize('gray') %}{{ stats[metric].gray }}{% endcall %}
|{{ stats[metric].total }}
||
{%- endfor %}
|#
{% endif %}

#|
||
**Test / control ID**
|**Name / ticket / dates**
|**Metric name**
|**Control**
|**Experiment**
|**diff / sbs result**
|**p-value / criteria**
||


{% for row in data_vertical %}
{% for res_entry in row.results %}

{% set criteria_results = res_entry.criteria_results or [None] %}

{% for crit_res in criteria_results %}
{% if row.is_first_in_obs and res_entry.is_first and loop.first %}
||**Observation {{ row.observation.id }}** {% if row.observation.tags %}(tags: {{ row.observation.tags | join(", ") }}){% endif %}||
{% endif %}
{% if (show_all_gray or (not row.all_gray)) and (show_same_color or (not row.same_color)) %}
||
{% if res_entry.is_first and loop.first %}
control: ((https://ab.yandex-team.ru/testid/{{ row.control.testid }} {{ row.control.testid }}))
{% if row.control.serpset_id %}(serpset {{ row.control.serpset_id }}){% endif %}
{% if row.control.sbs_system_id %}(sbs system id {{ row.control.sbs_system_id }}){% endif %}
{{ wiki_common.errors(row.control) }}

experiment: ((https://ab.yandex-team.ru/testid/{{ row.experiment.testid }} {{ row.experiment.testid }}))
{% if row.experiment.serpset_id %}(serpset {{ row.experiment.serpset_id }}){% endif %}
{% if row.experiment.sbs_system_id %}(sbs system id {{ row.experiment.sbs_system_id }}){% endif %}
{{ wiki_common.errors(row.experiment) }}

{% if row.discrepancy %}!!(red)(?Расхождение Часть метрик красная, а часть - зеленая?)!!{% endif %}
{% if row.split_changes %}!!(red)(?Переразбиение Даты переразбиений: {{ row.split_changes.keys() | sort | join(", ") }}?)!!{% endif %}
|<#{{ row.abt_exp_title }}#>
{{ row.ticket }}
{{ row.sbs_ticket }}
{{ row.observation.dates.start }} - {{ row.observation.dates.end }}
({{ row.observation.dates.number_of_days() }} days)
{% else %}
|
{% endif %}

{% if res_entry and loop.first %}
|{% call wiki_common.colorize(res_entry.color_by_value) %}**{{ res_entry.metric_key.pretty_name().replace('[', '\\[') }}**{% endcall %}
{% if res_entry.verdict_discrepancy %}!!Расхождение с вердиктом!!{% endif %}
|%%(wacko wrapper=text align=right)
{% call wiki_common.colorize(res_entry.color_by_value) %}
{% if res_entry.control_result %}
{{ res_entry.get_control_result() }}
{% else %}
No data!
{% endif %}
{% endcall %}
%%
|%%(wacko wrapper=text align=right)
{% call wiki_common.colorize(res_entry.color_by_value) %}
{% if res_entry.exp_result %}
{{ res_entry.get_exp_result() }}
{% else %}
No data!
{% endif %}
{% endcall %}
%%
|%%(wacko wrapper=text align=right)
{% call wiki_common.colorize(res_entry.color_by_value) %}
{% if res_entry.is_sbs_metric %}
{% if res_entry.sbs_result %}
{{ res_entry.get_sbs_result() }}
{% else %}
No data!
{% endif %}
{% else %}
{% if res_entry.diff %}
{{ utils.fmt(res_entry.diff.significant.abs_diff) }}
({{ utils.fmt(res_entry.diff.significant.perc_diff_human, 2) }}%)
{% else %}
No data!
{% endif %}
{% endif %}
{% endcall %}
%%
{% else %}
| | | |
{% endif %}

{% if crit_res %}
|
{% call wiki_common.colorize(res_entry.color_by_value) %}{{ utils.fmt(crit_res.pvalue) }}
{% if crit_res.criteria_key.pretty_name() != "unknown" %}{{ crit_res.criteria_key.pretty_name().replace('[', '\\[') }}{% endif %}
{% if crit_res.extra_data %}

{{ crit_res.extra_data }}
{% endif %}
{% endcall %}

{% else %}
|
{% endif %}

||
{% endif %}
{% endfor %}
{% endfor %}
{% endfor %}
|#
