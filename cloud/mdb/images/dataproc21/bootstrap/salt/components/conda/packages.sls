{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% do config_site.update({'conda': salt['grains.filter_by']({
    'Debian': {
        'boto3': '1.16.7',
        'botocore': '1.19.7',
        'ipykernel': '5.3.4',
        'ipython': '7.22.0',
        'jupyter_client': '6.1.12',
        'jupyter_core': '4.7.1',
        'koalas': '1.8.2',
        'matplotlib': '3.4.2',
        'numpy': '1.20.1',
        'pandas': '1.2.4',
        'pip': '21.0.1',
        'pyarrow': '4.0.0',
        'pyhive': '0.6.1',
        'python': '3.8.13',
        'requests': '2.25.1',
        'retrying': '1.3.3',
        'scikit-learn': '0.24.1',
        'seaborn': '0.11.1',
        'setuptools': '52.0.0',
        'urllib3': '1.25.11',
        'virtualenv': '20.4.1',
        'wheel': '0.36.2',
        'py4j': '0.10.9.2',
    },
}, merge=salt['pillar.get']('data:properties:conda'))}) %}