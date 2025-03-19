{% set config_site = {} %}

{% do config_site.update({'pip': salt['grains.filter_by']({
    'Debian': {
        'protobuf': '3.20.1',
        'yandexcloud': '0.166.0',
        'grpcio': '1.46.3'
    },
}, merge=salt['pillar.get']('data:properties:pip'))}) %}
