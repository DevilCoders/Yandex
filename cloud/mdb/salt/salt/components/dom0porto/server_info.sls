/etc/server_info.json:
    file.managed:
        - user: root
        - group: root
        - mode: 644
        - source: salt://{{ slspath }}/conf/server_info.py
        - template: py
