/etc/security/mongodb.conf:
    file.managed:
        - template: jinja
        - source: salt://{{slspath}}/conf/security-mongodb.conf
        - mode: 644
