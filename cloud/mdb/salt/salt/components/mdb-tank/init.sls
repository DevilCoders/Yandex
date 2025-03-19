'load-all':
  pkgrepo.managed:
    - refresh: true
    - onchanges_in:
        - cmd: repositories-ready
    - name: 'deb http://load-xenial.dist.yandex.ru/load-xenial stable/all/'
    - file: /etc/apt/sources.list
    - order: 3

'load-amd64':
  pkgrepo.managed:
    - refresh: true
    - onchanges_in:
        - cmd: repositories-ready
    - name: 'deb http://load-xenial.dist.yandex.ru/load-xenial stable/amd64/'
    - file: /etc/apt/sources.list
    - order: 4

mdb-tank-packages:
  pkg.installed:
    - pkgs:
        - mdb-tank-guns: '1.8739693'
        - yandex-tank-internal: '1.17.11'

/etc/yandex/mdb-tank:
  file.recurse:
    - user: root
    - group: root
    - source: salt://{{ slspath }}/configs

