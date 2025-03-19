/usr/local/bin/salt-autodeploy-monitor.py:
  file.managed:
    - source: salt://{{ slspath }}/files/salt-autodeploy-monitor.py
    - makedirs: True
    - follow_symlinks: False
    - mode: 755

{% set monitored = [] %}
{% set autodeploy_sls = salt["pillar.get"]("salt-autodeploy-sls", {}) %}
{% for sls_name, opts in autodeploy_sls.items() %}
  {% if opts and opts.get("monitor", False) %}
    {% do monitored.append(sls_name) %}
  {% endif %}
{% endfor %}

{% if monitored %}
/etc/monrun/salt-autodeploy/MANIFEST.json:
  file.serialize:
    - makedirs: True
    - serializer: json
    - serializer_opts:
      - indent: 2
    - dataset:
        version: 1
        checks:
          - check_script: /usr/local/bin/salt-autodeploy-monitor.py
            interval: 60
            args:
              - --monitor
              {%- for sls in monitored %}
              - {{sls}}
              {%- endfor %}
            run_always: true
            services:
              - salt-autodeploy
            timeout: 30
{% endif %}
 
/etc/cron.d/salt-autodeploy:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        SHELL=/bin/bash
        PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
        # Regular salt-call job see NOCDEV-6124 for details
        #                                               запуск конкретного sls файла
        #             field          allowed values          |
        #             -----          --------------          |
        #-------------minute         0–59                    |
        #     --------hour           0–23                    |  sls файл с конструкцией random.seed
        #     | ------day of month   1–31                    |          |
        #     | | ----month          1–12 (or names)         |          |
        #     | | | ,-day of week    0–7 (0 or 7 is Sun)     v          v
        #*/10 * * * * root salt-call                     state.sls  salt-by-percent                                               # TODO run salt-autodeploy-monitor via salt-venv
        */10 * * * *  root sleep $((RANDOM\%500)) && salt-call state.sls units.auto.include --output=json 2>>>(logger -t salt-autodeploy)|/usr/local/bin/salt-autodeploy-monitor.py --parse-stdin  2>>>(logger -t salt-autodeploy)
