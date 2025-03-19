/etc/ubic/service/mastermind-monolith.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/mastermind-monolith.json

mastermind-monolith-status:
  monrun.present:
    - command: RESULT=$(timeout 30 mastermind ping monolith 2>&1); [[ $RESULT == "0;Ok" ]] && echo '0;OK' || (echo "2;"; echo "${RESULT}" |tail -n1 ) | xargs
    - execution_interval: 300
    - execution_timeout: 200
    - type: mastermind
