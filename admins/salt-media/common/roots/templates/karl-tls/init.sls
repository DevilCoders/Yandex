karl:
  group.present:
    - name: karl
  user.present:
    - name: karl
    - optional_groups:
      - karl

/etc/karl/ssl/:
  file.directory:
  - user: karl
  - group: karl
  - dir_mode: 755
  - makedirs: True

{% for file in ['ca.crt', 'karl.crt', 'karl.key'] %}
/etc/karl/ssl/{{ file }}:
  file.managed:
    - contents_pillar: tls_karl:{{ file }}
    {% if grains['yandex-environment'] == 'testing' -%} 
    - mode: 644
    {%- else -%}
    - mode: 440
    {%- endif %}
    - user: karl
    - group: karl
    - makedirs: True
{% endfor %}

karl_gen_ssl_rehash:
  cmd.run:
    - name: c_rehash /etc/karl/ssl/
    - onchanges:
      - file: /etc/karl/ssl/*
