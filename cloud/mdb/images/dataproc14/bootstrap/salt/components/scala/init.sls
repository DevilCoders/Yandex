{% import 'components/hadoop/macro.sls' as m with context %}

scala-packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - scala
