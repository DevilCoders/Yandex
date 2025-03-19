/opt/yandex:
    file.directory:
        - mode: 755

{% set allCAs = salt['pillar.get']('internal.cert.ca.name', 'allCAs') %}
/opt/yandex/allCAs.pem:
    file.managed:
{% if salt['pillar.get']('internal.cert.ca') %}
        - contents_pillar: internal.cert.ca
{% else %}
        - source: salt://{{ slspath }}/conf/{{ allCAs }}.pem
{% endif %}
        - mode: 0644

/opt/yandex/gpnCAs.pem:
    file.absent

/opt/yandex/yaCAs.pem:
    file.absent

add-internal-CAs-to-system:
{% if salt['pillar.get']('data:running_on_template_machine', False) %}
    cmd.run:
{% else %}
    cmd.wait:
        - watch:
              - file: /opt/yandex/allCAs.pem
{% endif %}
        - name: |
            rm -f /usr/local/share/ca-certificates/*.crt \
            &&
            awk 'BEGIN {c=0;} /BEGIN CERTIFICATE/{c++} { print > "/usr/local/share/ca-certificates/allCAs-part-" c ".crt"}' < /opt/yandex/allCAs.pem \
            && update-ca-certificates
