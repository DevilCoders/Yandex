{% set config = pillar['unit_duty_bot'] %}
duty-bot-dependencies:
  pkg.installed:
    - name: python-arrow
    - name: libticket-parser2
    - name: libticket-parser2-python

duty-bot:
  file.managed:
    - name: /usr/local/bin/duty-bot.py
    - source: salt://{{ slspath }}/files/duty-bot.py
    - user: root
    - group: root
    - mode: 755

duty-bot-make-calendars:
  file.managed:
    - name: /usr/local/bin/make-calendars.sh
    - source: salt://{{ slspath }}/files/make-calendars.sh
    - user: root
    - group: root
    - mode: 755

duty-bot-logdir:
  file.directory:
    - name: /var/log/duty-bot/
    - user: root
    - group: root
    - mode: 775

duty-bot-logrotate:
  file.managed:
    - name: /etc/logrotate.d/duty-bot
    - contents: >
        /var/log/duty-bot/*.log {
          rotate 7
          weekly
          compress
          missingok
          notifempty
          delaycompress
        }
    - user: root
    - group: root
    - mode: 644

{% for bots, opts in config.items() %}
# Contains tokens. stored on salt master
{% set prefix = opts.get('prefix', '') %}
duty-bot{{prefix}}-config:
  yafile.managed:
    - name: /etc/yandex/duty-bot/config{{prefix}}.yaml
    - source: salt://{{ slspath }}/files/conf/config{{prefix}}.yaml
    - user: root
    - group: root
    - mode: 0400
    - makedirs: True
    - template: jinja

duty-bot{{prefix}}-cron:
  file.managed:
    - name: /etc/cron.d/duty-bot{{prefix}}
    - contents: >-
        {%- for action in opts.get('actions',[]) %}
        {{ action.get('schedule', '1-31/30 * * * *') }} root zk-flock duty-bot{{prefix}}-cron-{{action['name']}} "/usr/local/bin/duty-bot.py
        --sleep 30 -c /etc/yandex/duty-bot/config{{prefix}}.yaml 
        --state-file /var/log/duty-bot/duty-bot{{prefix}}.state 
        --log-file /var/log/duty-bot/duty-bot{{prefix}}.log {{action['name']}} 
        {%- for param in action.get('params',[]) %} {{param}} {% endfor -%}"
        2>/dev/null
        {% endfor %}
    - user: root
    - group: root
    - mode: 755

{% if opts['monrun'] %}
duty-bot{{prefix}}-monrun:
  monrun.present:
    - name: duty-bot{{prefix}}
    - command: test -f /var/log/duty-bot/duty-bot{{prefix}}.state && cat /var/log/duty-bot/duty-bot{{prefix}}.state || echo '0;ok'
    - execution_timeout: 10
    - execution_interval: 600
{% endif %}
{% endfor %}

duty-schedules-check:
  file.managed:
    - name: /usr/local/bin/duty-schedules-check.py
    - source: salt://{{ slspath }}/files/monrun/duty-schedules-check.py
    - user: root
    - group: root
    - mode: 755

duty-schedules-monrun:
  monrun.present:
    - name: duty-schedules
    - command: /usr/local/bin/duty-schedules-check.py
    - execution_timeout: 20
    - execution_interval: 3600
