{% set cluster = pillar.get('cluster') %}
{% set unit = 'syslog-ng' %}

{% for file in pillar.get('syslog-ng-files') %}
# replace symlinks with files
# workaround issue https://github.com/saltstack/salt/issues/31802
remove {{file}} symlink:
 file.absent:
   - name: {{file}}
   - onlyif:
     - test -L {{file}}

{{file}}:
  yafile.managed:
    - source: salt://files/{{ cluster }}{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - follow_symlinks: False  # must be enough for symlink replacement, but last update broke it
{% endfor %}

syslog-ng:
  service:
    - running
    - reload: True
