{% if "mongodb" in pillar %}
{% from slspath + '/map.jinja' import pingunoque with context %}

/etc/yandex/pingunoque/config.yaml:
  file.managed:
    - contents: |
        {{ pingunoque|yaml(False)|indent(8) }}
    - makedirs: True

pingunoque:
  service.running:
    - enable: True
    - watch:
      - file: /etc/yandex/pingunoque/config.yaml
    - require:
      - pkg: pingunoque_pkg
      - file: /etc/yandex/pingunoque/config.yaml

{% else %}


mongodb pillar NOT DEFINED:
  cmd.run:
    - name: echo "may be memory leaks on salt master!!!";exit 1

{% endif %}
