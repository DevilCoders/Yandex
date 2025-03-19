spacemimic:
  pkg:
   - installed
  group:
    - present
    - name: spacemimic
    - system: True
  user:
    - name: spacemimic
    - present
    - system: True
    - fullname: Space Mimic
    - shell: /bin/false
    - groups:
      - spacemimic

{% for file in pillar.get('spacemimic-obsolete-files') %}
{{ file }}:
  file.absent
{% endfor %}

{% for file in pillar.get('spacemimic-exec-files') %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('spacemimic-files') %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in pillar.get('spacemimic-dirs') %}
{{ file }}:
  file.directory:
    - mode: 755
    - user: spacemimic
    - group: spacemimic
    - makedirs: True
{% endfor %}

mimic:
  monrun.present:
    - command: "/usr/bin/jhttp.sh -n localhost -p 11000 -o -k -u /get/{{ pillar.get('spacemimic-ping-uri', '2739.yadisk:uploader.97099719030826319143612335895') }} -t 5"
    - execution_interval: 300
    - execution_timeout: 180
    - type: srw
