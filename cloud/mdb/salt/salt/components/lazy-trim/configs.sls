{% if salt['pillar.get']('data:lazy-trim:enabled', False) %}
lazy-trim-cronjob:
    file.managed:
        - name: /etc/cron.d/lazy-trim
        - template: jinja
        - source: salt://{{ slspath }}/conf/lazy-trim.cron
        - require:
            - pkg: lazy-trim-pkg
{% else %}
lazy-trim-cronjob:
    file.absent:
        - name: /etc/cron.d/lazy-trim
{% endif %}

lazy-trim-logrotate:
    file.managed:
        - name: /etc/logrotate.d/lazy-trim
        - template: jinja
        - source: salt://{{ slspath }}/conf/lazy-trim.logrotate
        - require:
            - pkg: lazy-trim-pkg
