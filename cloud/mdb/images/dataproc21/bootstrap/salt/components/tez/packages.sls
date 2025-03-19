{% import 'components/hadoop/macro.sls' as m with context %}

tez_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - tez: {{ m.version('tez') }}
