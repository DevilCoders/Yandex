{% set cluster = pillar.get('cluster') %}
{% set unit = 'rsync' %}

{% for file in pillar.get('rsync-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{ cluster }}/etc/rsyncd.conf
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

# FIXME: augeas or whole config management?
/etc/default/rsync:
  cmd.run:
    - name: sed -i 's/RSYNC_ENABLE=false/RSYNC_ENABLE=true/' /etc/default/rsync
    - unless: 'grep -q RSYNC_ENABLE=true /etc/default/rsync'

rsync:
  pkg.installed: []
  service.running:
    - require:
      - pkg: rsync
    - reload: True
