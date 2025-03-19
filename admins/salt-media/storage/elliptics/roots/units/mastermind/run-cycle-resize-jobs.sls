# MDS-14594

{% if 'elliptics-test-cloud' in grains['c'] %}
/usr/bin/run_cycle_resize_jobs.py:
  yafile.managed:
    - source: salt://{{ slspath }}/files/bin/run_cycle_resize_jobs.py
    - mode: 755
    - user: root
    - group: root

mastermind_run_cycle_resize_jobs:
  file.managed:
    - name: /etc/cron.d/mastermind_run_cycle_resize_jobs
    - contents: |
        * */12 * * * root zk-flock mastermind_run_cycle_resize_jobs "/usr/bin/run_cycle_resize_jobs.py --debug-logging --groupset-id-prefixes '1397601:1400946,1399100:1399240' --autoapprove  single_run" >/dev/null 2>/dev/null
    - user: root
    - group: root
    - mode: 755
{% endif %}
