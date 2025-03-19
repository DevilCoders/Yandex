{% import 'components/hadoop/macro.sls' as m with context %}

{% set packages = {
    'hadoop': 'any',
    'hadoop-client': 'any',
    'libssl-dev': 'any'
}
%}

hadoop_packages:
    pkg.installed:
        - refresh: False
        - pkgs:
{% for package, version in packages.items() %}
{% if version == 'any' %}
             - {{ package }}
{% else %}
             - {{ package }}: {{ version }}
{% endif %}
{% endfor %}
