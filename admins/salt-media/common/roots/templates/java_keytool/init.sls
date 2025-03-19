{% from slspath + '/map.jinja' import java_keytool with context %}

/var/lib/java_keytool:
  file.directory:
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True

{% for file in salt['cp.list_master'](prefix="/common/java_keytool") %}
{% set filename = file.split('/')[-1] %}
{{ filename }}_certificate:
  file.managed:
    - name: /var/lib/java_keytool/{{ filename }}
    - source: salt://{{ file }}
    - makedirs: True
    - user: root
    - group: root
    - file_mode: 0644

{{ filename }}_certificate_export:
  cmd.run:
    - name: keytool -trustcacerts -keystore {{ java_keytool.cacerts_path }} -storepass {{ java_keytool.storepass }} -noprompt -delete -alias {{ filename }} || true ; keytool -trustcacerts -keystore {{ java_keytool.cacerts_path }} -storepass {{ java_keytool.storepass }} -noprompt -importcert -alias {{ filename }} -file /var/lib/java_keytool/{{ filename }}
    - onchanges:
      - file: {{ filename }}_certificate
{% endfor %}
