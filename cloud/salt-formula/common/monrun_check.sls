monrun_{{ check_name }}:
  monrun.present:
    - name: {{ check_name }}
    - command: {{ command }}
    - execution_interval: {{ service['execution_interval']|default(60) }}
    - execution_timeout: {{ service['execution_timeout']|default(60) }}
    - type: {{ service['type']|default('nagios') }}
    - order: last # This is needed because monrun pkg should be installed prior to monrun.present state
                  # and we don't want to require monrun here to simplify includes,
                  # see https://github.yandex-team.ru/YandexCloud/salt-formula/pull/504
