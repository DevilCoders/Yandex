{% from slspath + "/map.jinja" import usr_envs with context %}

{% if "root" not in usr_envs.users %}
/root:
  file.recurse:
    - source: salt://{{ slspath }}/root
    - user: root
    - keep_symlinks: True
{% endif %}

{% for user in usr_envs.users %}
  {% if user == 'root' %}
    {% set home = '/root' %}
  {% else %}
    {% set home = '/home/' + user %}
  {% endif %}
{{ home }}:
  file.recurse:
    - source:
      - {{ usr_envs.spath }}{{ home }}
      - {{ usr_envs.spath }}/default
      - salt://{{ slspath }}/files
    - user: {{ user }}
    - onlyif:
      - test -d {{ home }}
    - keep_symlinks: True
{% endfor %}
