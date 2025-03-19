{% from "components/postgres/pg.jinja" import pg with context %}

{%
    set pkgs = {
        'postgresql-' + pg.version.major + '-plproxy': 'any',
        'mdb-pgcheck': '510-2092eaa',
        'python-flask': 'any',
        'python-uwsgi': 'any'
    }
%}

{% for package, version in pkgs.items() %}
{% if version == 'any' %}

{{ package }}:
    pkg.installed

{% else %}

{{ package }}:
    pkg.installed:
        - version: {{ version }}

{% endif %}
{% endfor %}
