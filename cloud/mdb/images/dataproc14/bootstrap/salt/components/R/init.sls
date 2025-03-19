{% import 'components/hadoop/macro.sls' as m with context %}

R-packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - r-base
