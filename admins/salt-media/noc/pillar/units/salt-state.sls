{% if grains['yandex-environment'] in ['testing'] and 'conductor' in grains and grains['conductor']['group'] != 'elliptics-test-dom0-lxd' %}
salt_state_command_args: "-w 4 -c 10 -a test=False"
{% else %}
salt_state_command_args: "-s 300 -t 600 -w 4 -c 10"
{% endif %}
salt_state_execution_interval: 3600
salt_state_execution_timeout: 600
