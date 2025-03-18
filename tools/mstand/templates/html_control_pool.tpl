{% extends "html_common.tpl" %}
{% import 'utils.tpl' as utils %}

{% block title %}check control pool{% endblock %}

{% block scripts %}
    <script type="text/javascript">
        {% include "common.js" %}

        document.addEventListener("DOMContentLoaded", function() {
            addVisibilityListener("cb-show-gray", ".control-pool__color-gray");
        });
        document.addEventListener("DOMContentLoaded", function() {
            addVisibilityListener("cb-show-separated-days", ".control-pool__separated-days");
        });
    </script>
    <!--suppress CssUnusedSymbol -->
    <style>
        .control-pool__color-red {
            background-color: #f44336; /* Material red 500 */
            color: white;
        }

        .control-pool__color-green {
            background-color: #2e7d32; /* Material green 800 */
            color: white;
        }

        .control-pool__color-yellow {
            background-color: #ffc107; /* Material amber 500 */
        }

        .control-pool__color-gray {
            color: #606060;
        }
    </style>
{% endblock %}

{% macro obs_group(group) %}
    <table>
        <tr class="bold">
            <td>Уровень значимости</td>
            <td>Прокрасов</td>
            <td>Доверительный интервал</td>
        </tr>
        {% for threshold, threshold_data in group.threshold_data | dictsort %}
            <tr
                    {% if not threshold_data["in"] %}
                        {% if threshold < 0.05 %}
                            class="error-box"
                        {% else %}
                            class="warning-box"
                        {% endif %}
                    {% endif %}
            >
                <td>{{ threshold }}</td>
                <td>{{ threshold_data.count }} / {{ group.length }}</td>
                <td><span class="hovertext" title="α = 0.05">[{{ threshold_data.left }}, {{ threshold_data.right }}]</span></td>
            </tr>
        {% endfor %}
    </table>

    <br/>

    <details>
        <summary>Подробно</summary>
        <table>
            <tr class="bold">
                <td>Наблюдение</td>
                <td>Даты</td>
                <td>Контроль</td>
                <td>Экcперимент</td>
                <td>Diff</td>
                <td>p-value</td>
            </tr>

            {% for obs in group.data %}
                <tr class="control-pool__color-{{ obs.colors[0.01] }}">
                    <td>{{ obs.obs_id }}</td>
                    <td>{{ obs.dates }}</td>
                    <td>{{ utils.fmt(obs.control_value) }}</td>
                    <td>{{ utils.fmt(obs.exp_value) }}</td>
                    <td>{{ utils.fmt(obs.diff.abs_diff) }} ({{ utils.fmt(obs.diff.perc_diff, 2) }}%)</td>
                    <td>{{ obs.pvalue }}</td>
                </tr>
            {% endfor %}
        </table>
    </details>
{% endmacro %}

{% block content %}
    <label>
        <input type="checkbox" id="cb-show-gray" name="show-gray" checked /> Показывать серые
    </label>
    <label>
        <input type="checkbox" id="cb-show-separated-days" name="show-separated-days" checked /> Показывать отдельные дни
    </label>

    {% for metric_key, metric_data in data.items() %}
        <h1>Метрика: {{ metric_key.pretty_name() }}</h1>
        <table>
            <h3>Длинные промежутки</h3>
            {{ obs_group(metric_data.long) }}
            <div class="control-pool__separated-days">
                <h3>Отдельные дни</h3>
                {{ obs_group(metric_data.one_day) }}
            </div>
        </table>
    {% endfor %}
{% endblock %}
