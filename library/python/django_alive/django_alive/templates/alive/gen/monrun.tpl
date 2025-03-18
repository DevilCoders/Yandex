{% if not checks %}
[{{ prefix }}-{{ group }}]
execution_interval=60
command={{ prog }} alivestate --monrun -g {{ group }}
docstring=alive check for group: {{ group }}
{% else %}
    {% for check in checks %}
[{{ prefix }}-{{ group }}-{{ check }}]
execution_interval=60
command={{ prog }} alivestate --monrun -g {{ group }} -c {{ check }}
docstring=alive check for group: {{ group }} and check: {{ check }}
    {% endfor %}
{% endif %}
