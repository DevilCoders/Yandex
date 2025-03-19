{% for file in pillar.get('mds-hide-conf-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
    - require:
      - pkg: mds-hide
      - pkg: zk-flock
{% endfor %}

{% for file in pillar.get('mds-hide-conf-dirs', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - require:
        - pkg: nginx
{% endfor %}

{% for pkg in pillar.get('mds-hide-additional-pkgs') %}
{{pkg}}:
  pkg:
    - installed
{% endfor %}
