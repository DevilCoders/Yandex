{% set lock_name = pillar.get('mastermind-namespace-space-lock-name', 'monrun-mastermind-namespace-space-' + grains.get("conductor")["group"]) %}
mastermind-namespace-space-check-script:
  pkg.installed:
    - pkgs:
      - python-numpy
      - python-arrow
      - python-yaml
  file.managed:
    - name: /usr/local/bin/mastermind-namespace-space.py
    - source: salt://files/mastermind-namespace-space-check/mastermind-namespace-space.py
    - user: root
    - group: root
    - mode: 755

mastermind-namespace-space-check:
  file.serialize:
    - name: /etc/monitoring/mastermind-namespace-space.yaml
    - dataset_pillar: mastermind-namespace-space
    - formatter: yaml
    - user: root
    - group: root
    - mode: 644

# replaced with cron below
  monrun.absent:
    - name: mastermind-namespace-space

mastermind-namespace-space-cron:
    file.managed:
      - name: /etc/cron.d/mastermind-namespace-space
      - contents: >
          13 * * * * root zk-flock {{lock_name}} '/usr/local/bin/mastermind-namespace-space.py -c /etc/monitoring/mastermind-namespace-space.yaml -f juggler-batch' | /usr/bin/juggler_queue_event --batch > /dev/null 2>&1 
      - user: root
      - group: root
      - mode: 755
