upload_hs_schema_data:
  file.managed:
    - name: /opt/mysql/.cache-schema/schema.sql
    - makedirs: true
    - source: salt://{{slspath}}/files/opt/mysql/.cache-schema/schema.sql
    - mode: 0400
upload_hs_schema_script:
  file.managed:
    - name: /opt/mysql/.cache-schema/apply.sh
    - makedirs: true
    - source: salt://{{slspath}}/files/opt/mysql/.cache-schema/apply.sh
    - mode: 0700
hs_schema_apply:
  cmd.run:
    - name: /opt/mysql/.cache-schema/apply.sh
    - unless: test -d /opt/mysql/cache/
