{% macro fmt(value, precision=20) %}{% if value or value == 0 %}{{ value | round(precision) }}{% else %}-{% endif %}{% endmacro %}

{% macro num_metrics_word(n) %}
{% if n == 1 %}
метрика
{% elif 2 <= n <= 4 %}
метрики
{% elif 5 <= n <= 20 or n == 0 %}
метрик
{% elif n < 100 %}
{{ num_metrics_word(n % 10) }}
{% else %}
{{ num_metrics_word(n % 100) }}
{% endif %}
{% endmacro %}

{% macro num_metrics(n) %}
{{ n }} {{ num_metrics_word(n) }}
{% endmacro %}
