{% import 'utils.tpl' as utils %}

{% macro build_filter_box() %}
    <label class="filter">
        Filter <input type="text" class="filter__text" autocomplete="on" />
        <span class="filter__help">
            <span class="hovertext">Filters help</span>
            <div class="filter__popup">
                <p>Examples:</p>
                <p class="example">color == 'red'</p>
                <p class="example">dateStart >= '2016-06-01' && expTicket.includes('EXPERIMENTS-8188')<p>
                <p class="example">metricName.includes('yt') || metricName.includes('yamr') && (pvalue <= 0.05 || diffPercent != 0)</p>
                <p class="example">observationTags.split(" ").includes("tag")</p>
                <p class="example">verdictDiscrepancy == 1</p>
                <p class="example">splitChange == 1</p>
                <p>Fields:</p>
                <p>controlTestid, expTestid, observationid, observationTags, expTicket, expName, metricName</p>
                <p>dateEnd, dateStart, days</p>
                <p>controlValue, expValue, diff, diffPercent, pvalue, color ('red','gray','green'), verdictDiscrepancy</p>
            </div>
        </span>
        <span class="filter__found" title="Metrics filtered"></span>
        <span class="filter__error"></span>
    </label>
{% endmacro %}


{% macro build_search_attrs(row, res_entry, crit_res) %}
	{% if res_entry.is_first %}data-is-first="1"{% endif %}
	data-observationid="{{row.observation.id}}"
	{% if row.control.testid %}data-control-testid="{{row.control.testid}}"{% endif %}
	{% if row.experiment.testid %}data-exp-testid="{{row.experiment.testid}}"{% endif %}
    {% if row.observation.tags %}data-observation-tags="{{ row.observation.tags | join(" ") }}"{% endif %}
	data-days="{{row.observation.dates.number_of_days()}}"

	data-date-start="{{row.observation.dates.start}}"

	data-date-end="{{row.observation.dates.end}}"

	data-exp-name="{{row.abt_exp_title}}"

    {% if row.ticket %}data-exp-ticket="{{row.ticket}}"{% endif %}

	data-metric-name="{{res_entry.metric_key.pretty_name()}}"

	{% if crit_res %}
        data-pvalue="{{utils.fmt(crit_res.pvalue)}}"
    {% endif %}

	{% if crit_res %}
        data-color="{{crit_res.color}}"
    {% else %}
        data-color="{{res_entry.color_by_value}}"
    {% endif %}

	{% if res_entry.control_result%}
	    {% if res_entry.is_sbs_metric %}
            data-exp-value="{{res_entry.control_result}}"
        {% else %}
	        data-control-value="{{utils.fmt(res_entry.control_result.metric_values.significant_value)}}"
        {% endif %}
	{% endif %}

	{% if res_entry.exp_result%}
        {% if res_entry.is_sbs_metric %}
            data-exp-value="{{res_entry.exp_result}}"
        {% else %}
	        data-exp-value="{{utils.fmt(res_entry.exp_result.metric_values.significant_value)}}"
	    {% endif %}
	{% endif %}

	{% if res_entry.diff %}data-diff="{{utils.fmt(res_entry.diff.significant.abs_diff)}}"{% endif %}

	{% if res_entry.diff %}data-diff-percent="{{utils.fmt(res_entry.diff.significant.perc_diff_human, 2)}}"{% endif %}

    {% if res_entry.verdict_discrepancy %}
        data-verdict-discrepancy=1
    {% else %}
        data-verdict-discrepancy=0
    {% endif %}

    {% if row.split_changes %}
        data-split-change=1
    {% else %}
        data-split-change=0
    {% endif %}

{% endmacro %}
