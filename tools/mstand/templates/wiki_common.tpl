{% macro colorize(color) %}
!!({{ color }}){{ caller() | trim }}!!
{% endmacro %}

{% macro metric_entry(color, title, value) %}
{% call colorize(color) %}
**{{ title }}:**
{{ value }}
{% endcall %}
{% endmacro %}

{% macro metric_stats(stats) %}
{{ metric_entry('green', 'green', stats.green) }}
{{ metric_entry('yellow', 'yellow', stats.yellow) }}
{{ metric_entry('red', 'red', stats.red) }}
{{ metric_entry('gray', 'gray', stats.gray) }}
{{ metric_entry('gray', 'total', stats.total) }}
{% endmacro %}

{% macro errors(exp) %}
{% if exp.errors %}!!(red)(?Ошибки {{ ", ".join(exp.errors) }}?)!!{% endif %}
{% endmacro %}

{% macro error_box(text) %}
{% if text %}
%%(wacko wrapper=page border="3px solid red")
{{ text }}
%%
{% endif %}
{% endmacro %}