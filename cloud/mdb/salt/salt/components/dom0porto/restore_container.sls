{% set target_container = salt['pillar.get']('target-container') %}

{% if not target_container %}
no-target-container-pillar:
    test.configurable_test_state:
        - result: False
        - comment: 'No target-container pillar value specified'
{% else %}
{%     if target_container not in salt['pillar.get']('data:porto', {}) %}
target-container-missing-in-pillar:
    test.configurable_test_state:
        - result: False
        - comment: 'Target container {{ target_container }} not found in pillar'
{%     else %}
{%         for container in salt['pillar.get']('data:porto', {}).keys() %}
{%             if container == target_container %}
{%                 for volume in salt['pillar.get']('data:porto:' + container + ':volumes', []) %}
restore-{{ volume['dom0_path'] }}:
    cmd.run:
        - name: mv {{ volume['dom0_path'] }}.*.bak {{ volume['dom0_path'] }}
        - unless:
            - ls {{ volume['dom0_path'] }}
{%                 endfor %}
{%             endif %}
{%         endfor %}
{%     endif %}
{% endif %}
