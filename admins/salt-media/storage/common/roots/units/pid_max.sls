{%- if grains['virtual'] == 'physical' %}
kernel.pid_max:
  sysctl.present:
    - value: 1048576
{%- endif %}
