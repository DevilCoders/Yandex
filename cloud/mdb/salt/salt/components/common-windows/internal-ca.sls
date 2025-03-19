yandex-dir:
    file.directory:
        - name: 'C:\ProgramData\Yandex'
        - user: 'SYSTEM'

yandex-ca-ceritficate:
    file.managed:
        - name: 'C:\ProgramData\Yandex\allCAs.pem'
{% if salt['pillar.get']('internal.cert.ca') %}
        - contents_pillar: internal.cert.ca
{% else %}
        - source: salt://components/common/conf/allCAs.pem
{% endif %}
        - require:
            - file: yandex-dir
