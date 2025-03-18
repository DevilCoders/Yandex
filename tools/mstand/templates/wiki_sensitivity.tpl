{% import 'utils.tpl' as utils %}
{% import 'wiki_common.tpl' as wiki_common %}

**Min pvalue** = {{ min_pvalue }}
**Threshold** = {{ threshold }}

#|
||
**Metric**
|**Sensitivity**
|**kstest**
{% for row in rows %}
{% set metric_key = metric_id_key_map[row['metric_id']] %}
|**{{ metric_key.pretty_name().replace('[', '\\[') }}**
{% endfor %}
||
{% for row in rows %}
{% set metric_id = row.metric_id %}
{% set metric_key = metric_id_key_map[row.metric_id] %}
||
**{{ metric_key.pretty_name().replace('[', '\\[') }}**
({{ row.pvalue_count }} pvalues)
|{{ row.sensitivity }}
|{{ row.kstest }}
{% for row_column in rows %}
{% set row_metric_id = row_column['metric_id'] %}
{% set row_metric_key = metric_id_key_map[row_metric_id] %}
|
{% if row_metric_id != metric_id %}
{% set row_metric_data = row.metrics[row_metric_id] %}
{% set color = row_metric_data.color %}
{{ wiki_common.metric_entry(color, 'p-value', row_metric_data.pvalue) }}
{{ wiki_common.metric_entry(color, 'diff', row_metric_data.diff) }}
{{ wiki_common.metric_entry(color, 'diff %', row_metric_data.diff_percent) }}
{% endif %}
{% endfor %}
||
{% endfor %}

|| | |
{% for metric_id in metric_id_key_map.keys() %}
|{{ wiki_common.metric_stats(stats[metric_id]) }}
{%- endfor %}
||
|#
