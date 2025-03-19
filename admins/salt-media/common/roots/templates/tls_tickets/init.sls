{% from slspath + "/map.jinja" import tls_tickets as t with context %}

tls_get_cron:
  file.managed:
    - name: /etc/cron.d/{{ t.cron.name }}
    - user: root
    - group: root
    - mode: 755
    - contents: |
        # get tickets from secdist put, rotate it /etc/nginx/ssl/tls/ and reload nginx
        ROBOT='{{t.robot.name}}'
        {{t.cron.time}} root sleep $((RANDOM\%900)) && /usr/sbin/get-secdist-keys.sh -R -r {{t.on_success}} ugc {{ t.srcpath }} {{ t.destdir }}
    - require:
      - file: /usr/sbin/get-secdist-keys.sh
      - file: {{ t.destdir }}

{% if t.robot.srckeydir %}
home_for_robot:
  file.directory:
    - name: /home/{{ t.robot.name }}/.ssh
    - user: {{ t.robot.name }}
    - mode: 700
    - makedirs: True
    - require_in:
      - file: robot_ssh_key
robot_ssh_key:
  file.managed:
    - name: /home/{{ t.robot.name }}/.ssh/id_rsa
{% if t.robot.ssh_key %}
    - contents: {{t.robot.ssh_key| json}}
{% else%}
    - source: salt://certs/{{ t.robot.srckeydir }}/{{ t.robot.key }}
{% endif %}
    - mode: 400
    - user: {{ t.robot.name }}
    - group: root
    - require_in:
      - cmd: initialize_keys
{% endif %}

/usr/sbin/get-secdist-keys.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/get-secdist-keys.sh
    - user: root
    - group: root
    - mode: 755

{{ t.destdir }}:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

initialize_keys:
  cmd.run:
    - name: |
        tn=/{{t.destdir}}/{{t.keyname}}
        /usr/sbin/get-secdist-keys.sh -R -f -r reload_nothing ugc {{t.srcpath}} {{t.destdir}};
        if test -s $tn; then
          dd if=<(base64 $tn) of=$tn.prev bs=1 count=48 2>/dev/null
          dd if=<(base64 $tn) of=$tn.prevprev bs=1 count=48 skip=1 2>/dev/null
          chmod 440 /{{t.destdir}}/*
          echo "changed=yes comment='Okay, i generate 3 tls_tickets'"
        else
          echo "changed=no comment='Can not initialize tickets!'";
          exit 1;
        fi
    - cwd: {{ t.destdir }}
    - stateful: True
    - env:
       - ROBOT: {{ t.robot.name }}
    - require:
      - file: /usr/sbin/get-secdist-keys.sh
      - file: {{ t.destdir }}
    - onlyif:
      - ((`ls -1 /{{t.destdir}}/{{t.keyname}}* 2>/dev/null|wc -l` < 3 ))

reload_nginx_by_tls_tickets:
  service.running:
    - name: nginx
    - reload: True
    - enable: True
    - watch:
      - cmd: initialize_keys

monitoring:
  monrun.present:
    - name: {{ t.monitoring.name }}
    - command: /usr/sbin/tls-tickets-monrun-check.sh
    - execution_interval: {{ t.monitoring.execution_interval }}
    - execution_timeout: {{ t.monitoring.execution_timeout }}
    - require:
      - file: monitoring
  file.managed:
    - name: /usr/sbin/tls-tickets-monrun-check.sh
    - source: salt://{{ slspath }}/files/tls-tickets-monrun-check.sh
    - mode: 755

/etc/monitoring/tls-tickets.conf:
  file.managed:
    - template: jinja
      contents: |
        TICKET_NAME="/{{t.destdir}}/{{t.keyname}}"
        AGO="{{t.monitoring.threshold_age}}"
