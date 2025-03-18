{% extends 'html_common.tpl' %}
{% import 'utils.tpl' as utils %}

{% block title %}compare sensitivity{% endblock %}

{% block content %}
    <b>Min pvalue</b>  = {{ min_pvalue }}
    <b>Threshold</b> = {{ threshold }}

    <table>
        <thead>
        <tr>
            <td>Metric name</td>
            <td>Sensitivity</td>
            <td>kstest</td>
            {% for row in rows %}
                {% set metric_key = metric_id_key_map[row['metric_id']] %}
                <td>{{ metric_key.pretty_name() }}</td>
            {% endfor %}
        </tr>
        </thead>
        <tbody>
        {% for row in rows %}
            {{ render_one_sensitivity_row(row, metric_id_key_map) }}
        {% endfor %}
        </tbody>
    </table>

    <br/>

    <b>Легенда</b><br/>
    <div class="colorbox" style="background-color: green;"></div> Метрика в строке чувствительнее<br/>
    <div class="colorbox" style="background-color: red;"></div> Метрика в столбце чувствительнее

    <br/>
    <br/>

    <table>
        <thead>
        <tr>
            <td>Metric</td>
            <td class="metric-green">Green</td>
            <td class="metric-red">Red</td>
            <td class="metric-gray">Gray</td>
        </tr>
        </thead>
        <tbody>
        {% for metric_id, stat_entry in stats.items() %}
            {% set metric_key = metric_id_key_map[metric_id] %}
            <tr>
                <td>{{ metric_key.pretty_name() }}</td>
                <td class="metric-green">
                    <span class="hovertext"
                          title="Эта метрика чувствительнее, чем {{ utils.num_metrics(stat_entry.green) }}">{{ stat_entry.green }}</span>
                </td>
                <td class="metric-red">
                    <span class="hovertext"
                          title="Эта метрика менее чувствительна, чем {{ utils.num_metrics(stat_entry.red) }}">{{ stat_entry.red }}</span>
                </td>
                <td class="metric-gray">{{ stat_entry.gray }}</td>
            </tr>
        {%- endfor %}
        </tbody>
    </table>
{% endblock %}


{% macro render_one_sensitivity_row(row, metric_id_key_map) %}
    {% set metric_key = metric_id_key_map[row.metric_id] %}
    <tr>
        <td> <b>{{ metric_key.pretty_name() }}</b><br>({{ row.pvalue_count }} pvalues)</td>
        <td class="numeric">{{ row.sensitivity }}</td>
        <td class="numeric">{{ row.kstest }}</td>
        {% for row_column in rows %}
            {% set row_metric_id = row_column['metric_id'] %}
            {% set row_metric_key = metric_id_key_map[row_metric_id] %}
            {% if row_metric_id == row.metric_id %}
                <td><span class="nodata">-</span></td>
            {% else %}
                {{ render_one_metric_in_row(metric_key, row.metrics[row_metric_id], row_metric_key) }}
            {% endif %}
        {% endfor %}
    </tr>
{% endmacro %}

{% macro render_one_metric_in_row(metric_key, row_metric_data, row_metric_key) %}
    <td class="nested-container">
        <table class="nested no-outer-border result-block metric-{{ row_metric_data.color }}">
            <tbody>
            <tr>
                <td>p-value</td>
                <td class="numeric">{{ utils.fmt(row_metric_data.pvalue) }}</td>
            </tr>
            <tr>
                <td>diff</td>
                <td class="numeric">
                    <span class="numeric hovertext"
                          title="S({{ metric_key.pretty_name() }}) - S({{ row_metric_key.pretty_name() }})">{{ utils.fmt(row_metric_data.diff) }}</span>
                </td>
            </tr>
            <tr>
                <td>diff %</td>
                <td class="numeric">
                    <span class="numeric hovertext"
                          title="diff / S({{ row_metric_key.pretty_name() }})">{{ utils.fmt(row_metric_data.diff_percent, 2) }}%</span>
                </td>
            </tr>
            <tr>
                <td>winner</td>
                <td>
                    <span class="bold">
                        {% if row_metric_data.color != "gray" %}
                            {% if row_metric_data.diff < 0 %}
                                {{ row_metric_key.pretty_name() }}
                            {% else %}
                                {{ metric_key.pretty_name() }}
                            {% endif %}
                        {% else %}
                            -
                        {% endif %}
                    </span>
                </td>
            </tr>
            </tbody>
        </table>
    </td>
{% endmacro %}

