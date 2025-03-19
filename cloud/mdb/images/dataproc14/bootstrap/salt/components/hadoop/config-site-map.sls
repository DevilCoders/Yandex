{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}
{%- set s3 = salt['pillar.get']('data:s3', {}) -%}

{% do config_site.update({'core': salt['grains.filter_by']({
    'Debian': {
        'fs.default.name': 'hdfs://' + masternode,
        'fs.defaultFS': 'hdfs://' + masternode,
        'io.file.buffer.size': 131072,
        'hadoop.tmp.dir': '/hadoop/tmp',
        'fs.s3a.aws.credentials.provider': 'ru.yandex.cloud.dataproc.s3.ComputeMetadataCredentialsProvider',
        'fs.s3a.signing-algorithm': 'YandexObjectStorageSigner',
        'fs.s3a.endpoint': s3.get('endpoint_url'),
        'fs.s3a.multiobjectdelete.enable': 'false'
    }
}, merge=salt['pillar.get']('data:properties:core'))}) %}
