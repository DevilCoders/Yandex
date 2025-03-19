{% set juggler_checks = '/var/lib/juggler-client/yc-monitors' %}

{{ juggler_checks }}/{{ service.file }}/MANIFEST.json:
  file.serialize:
    - makedirs: True
    - formatter: json
    - dataset:
        version: 1
        checks:
        - args: []
          check_script: {{ script }}/{{ service.file }}
          interval: {{ service.get('execution_interval', 60) }}
          run_always: true
          timeout: {{ service.get('execution_timeout', 60) }}
