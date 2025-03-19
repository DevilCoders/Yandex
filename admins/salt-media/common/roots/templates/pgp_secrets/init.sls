{% for file in salt['pillar.get']('pgp_secrets', [], merge=True) %}
{% set fname = file.keys()[0] %}
{{ fname }}:
  file.managed:
    - makedirs: True
    - user: {{ file[fname].get('user', 'root') }}
    - group: {{ file[fname].get('group', 'root') }}
    - mode: {{ file[fname].get('mode', '0440') }}
    - contents_newline: False
    - contents_pillar: pgp_secrets:{{ fname }}:data
{% endfor %}
