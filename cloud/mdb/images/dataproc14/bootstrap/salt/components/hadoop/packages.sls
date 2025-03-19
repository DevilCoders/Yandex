{% import 'components/hadoop/macro.sls' as m with context %}

{% set hadoop_version = m.version('hadoop') %}

{% set packages = {
    'hadoop': hadoop_version,
    'hadoop-client': hadoop_version,
    'libssl-dev': 'any',
    'hadoop-lzo': 'any'
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
