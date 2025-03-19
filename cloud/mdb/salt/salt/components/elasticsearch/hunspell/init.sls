elasticsearch-hunspell-req:
    test.nop:
        - require:
            - test: elasticsearch-conf-ready

elasticsearch-hunspell-ready:
    test.nop

/usr/local/yandex/es_configure_hunspell.sh:
    file.managed:
        - source: salt://{{ slspath }}/es_configure_hunspell.sh
        - mode: 755
        - makedirs: True

hunspell-packages:
    pkg.installed:
        - pkgs:
            - hunspell-hr
            - hunspell-lt
            - hunspell-tr
            - hunspell-no
            - hunspell-th
            - hunspell-it
            - hunspell-bn
            - hunspell-el
            - hunspell-ro
            - hunspell-es
            - hunspell-is
            - hunspell-pt-pt
            - hunspell-gu
            - hunspell-lo
            - hunspell-de-ch-frami
            - hunspell-bg
            - hunspell-si
            - hunspell-af
            - hunspell-de-de-frami
            - hunspell-kmr
            - hunspell-hi
            - hunspell-pt-br
            - hunspell-sl
            - hunspell-mn
            - hunspell-sk
            - hunspell-de-at-frami
            - hunspell-vi
            - hunspell-gug
            - hunspell-cs
            - hunspell-te
            - hunspell-ru
            - hunspell-da
            - hunspell-uk
            - hunspell-sr
            - hunspell-id
            - hunspell-gd
            - hunspell-pl
            - hunspell-oc
            - hunspell-hu
            - hunspell-en-gb
            - hunspell-ne
            - hunspell-sw
            - hunspell-en-za
            - hunspell-gl
            - hunspell-bs
            - hunspell-sv
            - hunspell-he
            - hunspell-kk
            - hunspell-be

hunspell-configuration:
    file.directory:
        - name: /etc/elasticsearch/hunspell
        - mode: 755
        - require:
            - test: elasticsearch-hunspell-req
    cmd.run:
        - name: bash /usr/local/yandex/es_configure_hunspell.sh
        - onchanges:
            - pkg: hunspell-packages
            - file: /etc/elasticsearch/hunspell
        - require:
            - test: elasticsearch-hunspell-req
            - pkg: hunspell-packages
            - file: /usr/local/yandex/es_configure_hunspell.sh
        - require_in:
              - test: elasticsearch-hunspell-ready
