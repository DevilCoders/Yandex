{% for file in ['/etc/profile.d/hw-watcher-storage.sh',
                '/etc/logrotate.d/yandex-hw-watcher-storage-config',
                '/etc/hw_watcher/conf.d/mds.conf'
                ] %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/src{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in ['/usr/lib/yandex/elliptics_hw_watcher/masterwrap.py',
                '/usr/lib/yandex/elliptics_hw_watcher/storage-hw-watcher-monrun.sh',
                '/etc/hw_watcher/hooks.d/disk-failed/01_elliptics_backup.sh',
                '/etc/hw_watcher/hooks.d/disk-replaced/01_elliptics_restore.sh',
                '/usr/bin/st-reset-sas-phy.sh'
                ] %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/src{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

storage-hw-watcher:
  monrun.present:
    - command: /usr/lib/yandex/elliptics_hw_watcher/storage-hw-watcher-monrun.sh disk link mem ecc cpu
    - execution_interval: 300
    - execution_timeout: 180

yandex-storage-shelf-hw-watcher-conf-depends:
  pkg.installed:
      - name: yandex-hw-watcher-shelf-tool-plugin
      - name: yandex-elliptics-common-scripts
      - name: yandex-hw-watcher
      - name: jq
