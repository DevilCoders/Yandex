{%- from slspath + "/map.jinja" import mm_vars with context -%}

{% if pillar.get('is_cloud', False) %}
/etc/ubic/service/mastermind-job-processor.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/mastermind-job-processor.json

mastermind-job-processor-status:
  monrun.present:
    - command: RESULT=$(timeout 30 mastermind ping job-processor 2>&1); [[ $RESULT == "0;Ok" ]] && echo '0;OK' || (echo "2;"; echo "${RESULT}" |tail -n1 ) | xargs
    - execution_interval: 300
    - execution_timeout: 200
    - type: mastermind

/etc/elliptics/mastermind/job_processor/job_processor.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/mastermind/job_processor/job_processor.conf
    - template: jinja
    - makedirs: true
    - context:
      vars: {{ mm_vars }}

/etc/elliptics/mastermind/job_processor/task_errors_policy.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/mastermind/job_processor/task_errors_policy.conf
    - template: jinja
    - makedirs: true
    - context:
      vars: {{ mm_vars }}
{% endif %}
