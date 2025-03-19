{%- for section_name, vars in config|dictsort %}
[{{ section_name }}]
{%- for k, v in vars|dictsort %}
{{ k }} = {{ v }}
{%- endfor %}
{% endfor %}
