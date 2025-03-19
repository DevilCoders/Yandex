{%- from slspath + "/map.jinja" import mm_vars with context -%}

{% if pillar.get('is_cloud', False) %}

stat-reporter:
  monrun.present:
    - command: "monrun-resizer-and-reporter.sh -u ping -p 8080 -l /var/log/mastermind/stat-reporter.log -r 1"
    - execution_interval: 60
    - execution_timeout: 30
    - type: mastermind

{% if mm_vars.get('stat_reporter', {}).get('billing', {}).get('reports_dump_path', None) %}
{{ mm_vars.stat_reporter.billing.reports_dump_path }}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

stat-reporter-cron:
  file.managed:
    - name: /etc/cron.d/stat-reporter
    - contents: |
        * */1 * * * root /usr/bin/mastermind-stat-reporter -c /etc/elliptics/mastermind.conf clenup-billing-reports --age-threshold=7d  >/dev/null 2>/dev/null
    - user: root
    - group: root
    - mode: 755
{% endif %}

{% endif %}
