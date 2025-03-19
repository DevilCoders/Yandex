systemd-journald-reload:
    cmd.run:
        - name: systemctl force-reload systemd-journald
        - onchanges:
            - file: /etc/systemd/journald.conf.d/00-rotate.conf

/etc/systemd/journald.conf.d:
    file.directory

/etc/systemd/journald.conf.d/00-rotate.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/journald-rotate.conf
        - template: jinja
        - require:
             - file: /etc/systemd/journald.conf.d

{% if salt['grains.get']('virtual', 'physical') == 'lxc' %}
console-getty.service:
    service.masked
{% endif %}
