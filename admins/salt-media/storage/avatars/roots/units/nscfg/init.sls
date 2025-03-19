{% for file in pillar.get('nscfg-files', [])  %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - user: root
    - group: root
    - mode: 644
    - template: jinja
{% endfor %}

/usr/local/ssl/certs:
  file.symlink:
    - target: /etc/ssl/certs
    - makedirs: true
