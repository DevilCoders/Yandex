/etc/config-monrun-cpu-check/config.yml:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        warning: 70
        critical: 80
        disabled: true

check_vms:
  cmd.run:
{# Should return 0 exit code if want to disable check #}
{# if exit code is non 0, then check must be turned ON #}
    - name: 'detect_vms.sh cpu_check; [[ $? == 0 ]] && sed -e "s/disabled: false/disabled: true/g" -i /etc/config-monrun-cpu-check/config.yml || sed -e "s/disabled: true/disabled: false/g" -i /etc/config-monrun-cpu-check/config.yml'
