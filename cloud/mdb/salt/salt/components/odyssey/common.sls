odyssey-reload:
    cmd.run:
        - name: service odyssey reload
        - onchanges:
            - file: /etc/odyssey/odyssey.conf
        - onlyif:
            - service odyssey status

/usr/local/yandex/odyssey-restart.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/odyssey-restart.sh
        - mode: 755
        - user: root
        - group: root
        - makedirs: True

