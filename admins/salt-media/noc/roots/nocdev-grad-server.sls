include:
  - units.rsyslog
  - units.nocdev-grads
  - units.juggler-checks.common
  - units.juggler-checks.redis-status
  - units.unified-agent
  - units.juggler-checks.unified-agent-status

mondata:
  pkg.installed: []

telegraf:
  service.running:
    - enable: True
    - reload: True
    - require:
        # тут нужно указывать не имя пакета, а id стейта, можно использовать
        # значение из `- name:` но для этого нужно использовать `- name:`
        # https://docs.saltproject.io/en/latest/ref/states/requisites.html
        - pkg: telegraf
    # в этом sls нет доступа до стейтов file с именами /etc/telegraf/*
    # поэтому такой watch не сработает, salt не следит за внешними файлами,
    # он следит только за стейтами управляющими этими файлами
    # https://docs.saltproject.io/en/latest/ref/states/requisites.html#requisite-matching
    # - watch:
    #     - file: /etc/telegraf/*
  pkg:
    - installed
    - pkgs:
      - telegraf-noc-conf
