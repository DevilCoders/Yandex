{% extends 'html_common.tpl' %}
{% import 'utils.tpl' as utils %}
{% import 'data_search.tpl' as data_search %}

{% macro ab_testid_link(testid) %}
    <a href="https://ab.yandex-team.ru/testid/{{ testid }}">{{ testid }}</a>
{% endmacro %}

{% macro ab_observation_link(observation) %}
    <a href="https://ab.yandex-team.ru/observation#obs_id={{observation}}">{{observation}}</a>
{% endmacro %}

{% macro ab_calc_link(observation, date_start, date_end) %}
    (<a href="https://ab.yandex-team.ru/observation/{{observation}}/launch/abt?date_start={{date_start}}&date_end={{date_end}}">calc in abt</a>)
{% endmacro %}

{% macro show_errors(exp) %}
{% if exp.errors %}<span class="warning hovertext" title="{{ ', '.join(exp.errors) }}">Ошибки</span><br/>{% endif %}
{% endmacro %}

{% if show_all_gray %}
{% set all_gray_class = "all-gray" %}
{% else %}
{% set all_gray_class = "all-gray hidden" %}
{% endif %}

{% if show_same_color %}
{% set same_color_class = "same-color" %}
{% else %}
{% set same_color_class = "same-color hidden" %}
{% endif %}

{% block title %}compare metrics{% endblock %}
{% block scripts %}
    <script type="text/javascript">
        {% include "common.js" %}
        {% include "filter.js" %}
        {% include "sort_table.js" %}

        document.addEventListener("DOMContentLoaded", function() {
            addVisibilityListener("cb-show-gray", ".all-gray");
            addVisibilityListener("cb-show-same-color", ".same-color");

            var warningBox = document.getElementById("warning-box");
            var closeButton = document.getElementById("warning-close");
            if (closeButton) {
                closeButton.addEventListener("click", function () {
                    warningBox.classList.add("hidden");
                });
            }

            var textFilter = document.querySelector('.filter__text');

            function setFilter(value) {
                textFilter.value = value;
                filterResults(value);
            }

            function setTagFilter(tag) {
                setFilter('observationTags.split(" ").includes("' + tag + '")');
            }

            window.setTagFilter = setTagFilter;

            function setMetricNameFilter(metricName) {
                setFilter('metricName == "' + metricName + '"');
            }

            window.setMetricNameFilter = setMetricNameFilter;

            addFilterListener(textFilter);
            filterResults('');
            var searchExamples = document.getElementsByClassName("example");
            Array.prototype.forEach.call(searchExamples, function(el) {
                el.addEventListener('click', function() {
                    setFilter(el.firstChild.nodeValue);
                });
            });

            makeSortable(document.querySelector('.stats_table'));
        });
    </script>
{% endblock %}

{% block content %}
    {% if validation_errors %}
        <div id="warning-box" class="warning-box">
            <span id="warning-close">&times;</span>
            <b>Pool validation detected errors!</b>
            <pre>{{ validation_errors }}</pre>
        </div>
    {% endif %}

    {% if red_lamps %}
        <div id="warning-box" class="warning-box">
            <span id="warning-close">&times;</span>
            <b>Загорелись лампочки (см. под катом)</b>
        </div>
    {% endif %}
    <b>Threshold</b> = {{ threshold }}
    <label>
        <input type="checkbox" id="cb-show-gray" name="show-gray" {% if show_all_gray %} checked {% endif %}/> Show all-gray experiments
    </label>
    <label>
        <input type="checkbox" id="cb-show-same-color" name="show-same-color" {% if show_same_color %} checked {% endif %}/> Show same-color experiments
    </label>
    <label>
        <input type="checkbox" id="cb-show-only-diff-color" name="show-only-diff-color"/> Show only diff-color experiments after filters
    </label>
    <br />

    {% if source_url %}
		<a href="{{ source_url }}">Operation URL</a><br/>
        <br />
    {% endif %}

    {% if stats %}
        <br />
        {{ stats_table(stats) }}
        <br />
        <br />
    {% endif %}

    {{ data_search.build_filter_box() }}
    {{ build_table(data_vertical) }}

    <br />
    <br />

    <details>
        <summary id="lamps_table">Лампочки</summary>
        {{ stats_table(lamp_stats) }}
        <br/>
        <br/>
        {{ build_table(lamp_data_vertical) }}
    </details>

    <br />
    <br />

{% endblock %}


{% macro build_table(data_source) %}
    <table class="metrics">
        <thead>
        <tr>
            <td>Experiment / Control IDs</td>
            <td>Name / ticket / dates</td>
            <td>Metric name</td>
            <td>Value</td>
            <td>p-value / criteria</td>
        </tr>
        </thead>
        <tbody>
        {% for row in data_source %}
            {% for res_entry in row.results %}
                {% if res_entry.control_result %}
                    {% set criteria_results = res_entry.criteria_results or [None] %}
                    {% for crit_res in criteria_results %}
                        {% set is_first_crit = loop.first %}
                        {% if row.is_first_in_obs and res_entry.is_first and is_first_crit %}
                        <tr>
                            <td colspan="99">
                                <strong>Observation {{ ab_observation_link(row.observation.id) }}</strong>
                                {% if row.observation.id %}
                                    {{ ab_calc_link(row.observation.id,
                                                    format_date(row.observation.dates.start),
                                                    format_date(row.observation.dates.end)) }}
                                {% endif %}
                                {% if row.observation.tags %}
                                    {% for tag in row.observation.tags %}
                                        <span class="tag" title="Щелкни, чтобы отфильтровать по тегу" onclick="setTagFilter('{{ tag }}')">{{ tag }}</span>
                                    {% endfor %}
                                {% endif %}
                            </td>
                        </tr>
                        {% endif %}
                        <tr class="metrics__row{% if row.all_gray %} {{ all_gray_class }}{% endif %}{% if row.same_color %} {{ same_color_class }}{% endif %}"
                            {{ data_search.build_search_attrs(row, res_entry, crit_res) }}>
                            {{ build_one_metric_row(row, res_entry, crit_res, is_first_crit) }}
                        </tr>
                    {% endfor %}
                {% endif %}
            {% endfor %}
        {% endfor %}
        </tbody>
    </table>
{% endmacro %}

{% macro abt_experiment_pair_info(row) %}
	<table class="nested no-outer-border">
        <tbody>
            <tr>
                <td colspan="2" class="bold">Control</td>
            </tr>
            {% if row.control.testid %}
                <tr>
                    <td>testid</td>
                    <td>{{ ab_testid_link(row.control.testid) }}</td>
                </tr>
            {% endif %}
            {% if row.control.serpset_id %}
                <tr>
                    <td>serpset id</td>
                    <td>{{ row.control.serpset_id }}</td>
                </tr>
            {% endif %}
            {% if row.control.sbs_system_id %}
                <tr>
                    <td>sbs system id</td>
                    <td>{{ row.control.sbs_system_id }}</td>
                </tr>
            {% endif %}
            {% if row.control.errors %}
                <tr>
                    <td colspan="2">{{ show_errors(row.observation.control) }}</td>
                </tr>
            {% endif %}
            <tr>
                <td colspan="2" class="bold">Experiment</td>
            </tr>
            {% if row.experiment.testid %}
                <tr>
                    <td>testid</td>
                    <td>{{ ab_testid_link(row.experiment.testid) }}</td>
                </tr>
            {% endif %}
            {% if row.experiment.serpset_id %}
                <tr>
                    <td>serpset id</td>
                    <td>{{ row.experiment.serpset_id }}</td>
                </tr>
            {% endif %}
            {% if row.experiment.sbs_system_id %}
                <tr>
                    <td>sbs system id</td>
                    <td>{{ row.experiment.sbs_system_id }}</td>
                </tr>
            {% endif %}
            {% if row.experiment.errors %}
                <tr>
                    <td colspan="2">{{ show_errors(row.experiment) }}</td>
                </tr>
            {% endif %}
            {% if row.discrepancy %}
                <tr>
                    <td colspan="2">
                        <span class="warning hovertext" title="Часть метрик красная, а часть - зеленая">Расхождение</span>
                    </td>
                </tr>
            {% endif %}
            {% if row.split_changes %}
                <tr>
                    <td colspan="2">
                        <details>
                            <summary class="warning">Переразбиение</summary>
                            <table class="nested">
                                {% for date, testids in row.split_changes | dictsort %}
                                    <tr><td class="bold" colspan="2">{{ date }}</td></tr>
                                    {% for testid, data in testids | dictsort %}
                                        <tr>
                                            <td>{{ testid }}</td>
                                            <td>
                                                {% if data.manual %}ручное{% endif %}
                                                {% if data.regular %}регулярное{% endif %}
                                            </td>
                                        </tr>
                                    {% endfor %}
                                {% endfor %}
                            </table>
                        </details>
                    </td>
                </tr>
            {% endif %}
        </tbody>
	</table>
{% endmacro %}


{% macro st_ticket_info(row) %}
	<div>
	{{ row.abt_exp_title }}<br/>
	{% if row.ticket %}
		<a href="https://st.yandex-team.ru/{{ row.ticket }}">{{ row.ticket }}</a><br/>
	{% endif %}
	{% if row.sbs_ticket %}
		<a href="https://st.yandex-team.ru/{{ row.sbs_ticket }}">{{ row.sbs_ticket }}</a><br/>
	{% endif %}
	{{ row.observation.dates.start }} - {{ row.observation.dates.end }}
	({{ row.observation.dates.number_of_days() }} days)
	</div>
{% endmacro %}


{% macro metric_values_pair_info(res_entry) %}
	<table class="nested no-outer-border result-block metric-{{ res_entry.color_by_value }}">
        <tbody>
            <tr>
                <td>control</td>
                <td class="numeric">
                    {% if res_entry.control_result %}
                        {{ res_entry.get_control_result() }}
                    {% else %}
                        <span class="nodata">No data!</span>
                    {% endif %}
                </td>
            </tr>
            <tr>
                <td>experiment</td>
                <td class="numeric">
                    {% if res_entry.exp_result %}
                        {{ res_entry.get_exp_result() }}
                    {% else %}
                        <span class="nodata">No data!</span>
                    {% endif %}
                </td>
            </tr>
            {% if res_entry.is_sbs_metric %}
                <tr>
                    <td>sbs result</td>
                    <td class="numeric">
                        {% if res_entry.sbs_result %}
                            {{ res_entry.get_sbs_result() }}
                        {% else %}
                            <span class="nodata">No data!</span>
                        {% endif %}
                    </td>
                </tr>
            {% else %}
                <tr>
                    <td>diff</td>
                    <td class="numeric">
                        {% if res_entry.diff %}
                            {{ utils.fmt(res_entry.diff.significant.abs_diff) }}
                        {% else %}
                            <span class="nodata">No data!</span>
                        {% endif %}
                    </td>
                </tr>
                <tr>
                    <td>diff %</td>
                    <td class="numeric">
                        {% if res_entry.diff %}
                            {{ utils.fmt(res_entry.diff.significant.perc_diff_human, 2) }}%
                        {% else %}
                            <span class="nodata">No data!</span>
                        {% endif %}
                    </td>
                </tr>
            {% endif %}
        </tbody>
	</table>
{% endmacro %}


{% macro build_one_metric_row(row, res_entry, crit_res, is_first_crit) %}
	<td class="nested-container metric__meta{% if not res_entry.is_first or not is_first_crit %} metric__meta_additional{% endif %}">
		{{ abt_experiment_pair_info(row) }}
	</td>
	<td class="metric__meta{% if not res_entry.is_first or not is_first_crit %} metric__meta_additional{% endif %}">
		{{ st_ticket_info(row) }}
	</td>

	{% if res_entry and is_first_crit %}
		<td class="bold metric-{{ res_entry.color_by_value }}">
		    <span title="Щелкни, чтобы отфильтровать по имени метрики"
		          style="cursor: pointer"
		          onclick="setMetricNameFilter('{{ res_entry.metric_key.pretty_name() }}')">
		        {{ res_entry.metric_key.pretty_name() }}
		    </span>
            {% if not res_entry.is_sbs_metric and res_entry.color_by_rows == "red" %}
                {% set cont_rows = res_entry.control_result.metric_values.row_count %}
                {% set exp_rows = res_entry.exp_result.metric_values.row_count %}
                <br/>
                <span class="warning hovertext"
                      title="контроль: {{ cont_rows }}, эксперимент: {{ exp_rows }}">Разное количество пользователей</span>
            {% endif %}
            {% if res_entry.verdict_discrepancy %}
                <br/>
                <span class="warning hovertext"
                      title="метрика прокрасила ухудшающий эксперимент в зеленый">Расхождение с вердиктом</span>
            {% endif %}
		<td class="nested-container">
			{{ metric_values_pair_info(res_entry) }}
		</td>
	{% else %}
		<td></td>
		<td></td>
	{% endif %}

	{% if crit_res %}
		<td class="metric-{{ crit_res.color }}">
			<span class="numeric">{{ utils.fmt(crit_res.pvalue) }}</span><br/>
			{% if crit_res.criteria_key.pretty_name() != "unknown" %}
				<br> {{ crit_res.criteria_key.pretty_name() }}
			{% endif %}
			{% if crit_res.extra_data %}
				<br> {{ crit_res.extra_data }}
			{% endif %}
		</td>
	{% else %}
		<td></td>
	{% endif %}

{% endmacro %}
