{% set config_site = {} %}

{% do config_site.update({'pip': salt['grains.filter_by']({
    'Debian': {
        'protobuf': '3.17.1',
        'yandexcloud': '0.85.0',
        'grpcio': '1.36.1'
    },
}, merge=salt['pillar.get']('data:properties:pip'))}) %}
