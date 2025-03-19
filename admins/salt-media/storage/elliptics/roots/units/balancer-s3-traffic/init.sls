/usr/local/bin/balancer-s3-traffic.py:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/balancer-s3-traffic.py
    - mode: 755
    - user: root
    - group: root
    - template: jinja

/etc/logrotate.d/balancer-s3-traffic:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/balancer-s3-traffic.logrotate
    - user: root
    - group: root
    - mode: 755

balancer-s3-traffic-cron:
    file.managed:
      - name: /etc/cron.d/balancer-s3-traffic
      - contents: >
          22 * * * * root zk-flock balancer-s3-traffic '/usr/local/bin/balancer-s3-traffic.py -f juggler-batch' | /usr/bin/juggler_queue_event --batch > /dev/null 2>&1 
      - user: root
      - group: root
      - mode: 755

balancer-s3-top-buckets:
  monrun.present:
    - command: /usr/local/bin/balancer-s3-traffic.py -f monrun
    - execution_interval: 1800
    - execution_timeout: 1500

python-boto:
  pkg:
   - installed
