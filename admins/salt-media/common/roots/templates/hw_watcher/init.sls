{% if grains['os_family'] == "Debian" %}
{% from slspath + '/map.jinja' import hw_watcher,cluster with context %}

/etc/hw_watcher/hw_watcher.conf:
  file.managed:
    - source: 
      - salt://files/{{cluster}}/etc/hw_watcher/hw_watcher.conf
      - salt://{{cluster}}/etc/hw_watcher/hw_watcher.conf
      - salt://{{ slspath }}/files/etc/hw_watcher/hw_watcher.conf
    - template: jinja
    - user: root
    - group: root
    - context:
      hw_watcher: {{ hw_watcher }}
    - mode: 644
    - makedirs: True

yandex-hw-watcher:
  pkg.installed
{% endif %}
