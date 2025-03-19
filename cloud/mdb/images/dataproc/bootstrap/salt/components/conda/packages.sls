{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% do config_site.update({'conda': salt['grains.filter_by']({
    'Debian': {
        'boto3': '1.16.7',
        'botocore': '1.19.7',
        'ipykernel': '5.3.4',
        'ipython': '7.19.0',
        'grpc-cpp': '1.36.1',
        'grpcio': '1.36.1',
        'jupyter': '1.0.0',
        'koalas': '1.7.0',
        'matplotlib': '3.2.2',
        'numpy': '1.19.2',
        'pandas': '1.1.3',
        'pip': '20.2.4',
        'pyarrow': '1.0.1',
        'pyhive': '0.6.1',
        'python': '3.8.10',
        'requests': '2.24.0',
        'retrying': '1.3.3',
        'scikit-learn': '0.23.2',
        'seaborn': '0.10.0',
        'setuptools': '50.3.1',
        'urllib3': '1.25.11',
        'virtualenv': '20.2.1',
        'wheel': '0.35.1',
        'py4j': '0.10.9.2',
    },
}, merge=salt['pillar.get']('data:properties:conda'))}) %}
