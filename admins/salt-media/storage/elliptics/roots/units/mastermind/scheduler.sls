{% if pillar.get('is_cloud', False) %}
/etc/ubic/service/mastermind-scheduler.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/mastermind-scheduler.json

mastermind-scheduler-status:
  monrun.present:
    - command: RESULT=$(timeout 130 mastermind ping scheduler --timeout 120 2>&1); [[ $RESULT == "0;Ok" ]] && echo '0;OK' || (echo "2;"; echo "${RESULT}" |tail -n1 ) | xargs
    - execution_interval: 300
    - execution_timeout: 200
    - type: mastermind
{% endif %}
