mavrodi_user:
  user.present:
    - name: mavrodi
  group.present:
    - name: mavrodi

/etc/mavrodi/ssl/:
  file.directory:
  - user: mavrodi
  - group: mavrodi
  - dir_mode: 755
  - makedirs: True

{% for file in ['ca.crt', 'mavrodi.crt', 'mavrodi.key'] %}
/etc/mavrodi/ssl/{{ file }}:
  file.managed:
    - contents_pillar: tls_karl:{{ file }}
    {% if grains['conductor']['project'] == 'disk' -%}
    - mode: 440
    {%- else -%}
    - mode: 444
    {%- endif %}
    - user: mavrodi
    - group: mavrodi
    - makedirs: True
{% endfor %}

mavrodi_gen_ssl_rehash:
  cmd.run:
    - name: c_rehash /etc/mavrodii/ssl/
    - onchanges:
      - file: /etc/mavrodi/ssl/*
