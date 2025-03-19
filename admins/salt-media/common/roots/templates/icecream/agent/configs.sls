{% from slspath + "/map.jinja" import agent with context %}
include:
  - .service

/etc/yandex/icecream-agent/config.json:
  file.serialize:
    - dataset: {{ agent.config }}
    - formatter: json
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True

