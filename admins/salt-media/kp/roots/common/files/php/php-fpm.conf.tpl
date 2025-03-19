{% load_yaml as pool_default %}
user: 'www-data'
group: 'www-data'
listen: '127.0.0.1:9000'
pm: 'dynamic'
'pm.max_children': 5
'pm.start_servers': 2
'pm.min_spare_servers': 1
'pm.max_spare_servers': 3
{% endload -%}
{% for section_name, vars in config|dictsort -%}
{% if section_name == 'global' -%}
{% set mvars=vars -%}
{% else -%}
{% set mvars=pool_default -%}
{% do mvars.update(vars) -%}
{% endif %}
[{{ section_name }}]
{% for k, v in mvars|dictsort -%}
{{ k }} = {{ v }}
{% endfor -%}
{% endfor %}
