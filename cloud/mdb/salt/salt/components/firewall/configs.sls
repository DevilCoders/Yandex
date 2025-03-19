{% set default_file_list = [
    '00_default_ip4.conf',
    '00_default_ip6.conf',
]%}
/etc/ferm/ferm.conf:
  file.managed:
      - template: jinja
      - source: salt://components/firewall/conf/ferm.conf
      - makedirs: True
      - require_in:
          - test: ferm-ready
      - watch_in:
          - cmd: reload-ferm-rules

/etc/init.d/ferm:
  file.managed:
      - template: jinja
      - makedirs: True
      - mode: 755
      - source: salt://components/firewall/conf/ferm.init.ubuntu
      - require_in:
          - test: ferm-ready

{% if salt.grains.get('osrelease') == '18.04' %}
/lib/systemd/system/ferm.service:
  file.managed:
      - makedirs: True
      - mode: 644
      - source: salt://components/firewall/conf/ferm.service
      - onchanges_in:
        - module: systemd-reload
      - require_in:
          - test: ferm-ready
      - require:
        - pkg: firewall-packages
{% endif %}

/usr/local/yandex/ferm_helper.py:
  file.managed:
      - makedirs: True
      - mode: 755
      - source: salt://components/firewall/conf/ferm_helper.py
      - require_in:
        - pkg: firewall-packages

# Default rules
{% for file in default_file_list %}
/etc/ferm/conf.d/{{ file }}:
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/conf.d/{{file}}
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
{% endfor %}
