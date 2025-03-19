{% set read_ahead_kb = salt.pillar.get('data:dbaas_compute:read_ahead_kb', 128) %}
{% for disk in salt.grains.get('disks') %}
disable_read_ahead_{{ disk }}:
    cmd.run:
        - name: "echo {{ read_ahead_kb }} > /sys/block/{{ disk }}/queue/read_ahead_kb"
        - unless:
            - "grep -q '^{{ read_ahead_kb }}$' /sys/block/{{ disk }}/queue/read_ahead_kb"
{% endfor %}

/etc/systemd/system/read-ahead.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/read-ahead.unit
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

read-ahead-unit-enabled:
    service.enabled:
        - name: read-ahead
        - require:
            - module: systemd-reload
            - file: /etc/systemd/system/read-ahead.service
