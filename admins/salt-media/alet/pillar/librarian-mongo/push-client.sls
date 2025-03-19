{% set stats=["logstore01", "logstore02"] %}

push_client:
  clean_push_client_configs: True
  stats:
    {% for st in stats %}
    - name: {{ st }}
      port: 8080
      logs:
        - file: mongodb/mongodb.log
    {% endfor %}
