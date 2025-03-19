{% from slspath + "/map.jinja" import push_client with context %}
include:
  - .services

/usr/local/bin/push-client-check.sh:
  file.managed:
    - source: {{ push_client.check.script }}
    - mode: "0755"
    - require:
      - pkg: push_client_packages
      - file: /etc/monrun/conf.d/push-client.conf

/etc/monrun/conf.d/push-client.conf:
  file.managed:
    - mode: "0644"
    - template: jinja
      contents: |
        [{{push_client.check.get("name", "push-client-status")}}]
        execution_interval={{push_client.check.get("interval", 300)}}
        command=/usr/local/bin/push-client-check.sh

/etc/monitoring/push-client-status.conf:
  file.managed:
    - mode: "0644"
    - makedirs: True
    - template: jinja
      contents: |
        # Status parameters
        {%- if "status" in push_client.check %}
          {%- set s = push_client.check.get("status") %}
          {%- if s and s is mapping %}
            {%- for name, value in s.items() %}
        CHECK_{{name|upper()}}={{value}}
            {%- endfor %}
          {%- else %}
            {%- if not s %}
        CHECK_STATUS=False
        {%- endif %}{% endif %}{% endif %}
        # Logs parameters
        {%- if "logs" in push_client.check %}
          {%- set l = push_client.check.get("logs") %}
          {%- if l and l is mapping %}
            {%- for name, value in l.items() %}
        CHECK_LOGS_{{name|upper()}}={{value}}
            {%- endfor %}
          {%- else %}
            {%- if not l %}
        CHECK_LOGS=False
        {%- endif %}{% endif %}{% endif %}
