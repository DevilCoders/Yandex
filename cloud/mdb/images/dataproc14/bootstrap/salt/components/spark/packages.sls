{% import 'components/hadoop/macro.sls' as m with context %}

{% set packages = {
    'masternode': {
        'spark-history-server': 'any'
    }
} %}

spark_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - spark-core
            - spark-external
            - spark-python
            - spark-sparkr
            - spark-datanucleus
            - spark-yarn-shuffle

{{ m.pkg_present('spark-history', packages) }}
