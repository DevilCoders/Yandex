{% set target_path = salt['pillar.get']('target-dom0-path') %}
{% set token = salt['pillar.get']('delete-token') %}

{% if target_path and token %}
delete-target-path-backup-{{ target_path }}:
    cmd.run:
        - name: rm -rf {{ target_path }}.*.bak
        - onlyif:
            - ls {{ target_path }}.*.bak
        - require_in:
            - cmd: report-success-{{ target_path }}

report-success-{{ target_path }}:
    cmd.run:
        - name: |
            curl -f -i -X POST {{ salt['pillar.get']('data:config:dbm_url') }}/api/v2/dom0/volume-backup-delete-report/{{ salt['grains.get']('id') }}?path={{ target_path }} \
                --cacert /opt/yandex/allCAs.pem \
                -H 'Accept: application/json' -H 'Content-Type: application/json' \
                -d '{"token": "{{ token }}"}'
{% else %}
invalid-delete-volume-backup-pillar:
    test.fail_without_changes
{% endif %}
