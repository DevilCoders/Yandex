elliptics-tls-add-karl-group:
  group.present:
    - name: karl
  user.present:
    - name: karl
    - optional_groups:
      - karl

{% for file in ['RootStorageCA.crt', 'storage.crt', 'storage.key'] %}
/etc/elliptics/ssl/{{ file }}:
  file.managed:
    - contents_pillar: tls_elliptics:{{ file }}
    {% if grains['conductor']['project'] == 'disk' and file == 'storage.key' -%}
    - mode: 440
    {%- else -%}
    - mode: 644
    {%- endif %}
    - user: root
    - group: karl
    - makedirs: True
{% endfor %}

gen_ssl_rehash:
  cmd.run:
    - name: c_rehash /etc/elliptics/ssl/
    - onchanges:
      - file: /etc/elliptics/ssl/*

openssl:
  pkg:
   - installed
