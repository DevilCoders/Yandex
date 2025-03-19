{% import 'components/hadoop/macro.sls' as m with context %}

{% set packages = {
    'masternode': {
        'livy': 'any'
    }
} %}

{% set service_packages = {
    'masternode': {
        'livy-server': 'any'
    }
} %}

{{ m.pkg_present('livy', packages) }}

# This hack will only download packages for yarn services
# For actual install see services.sls
{% if salt['ydputils.is_presetup']() %}
{{ m.pkg_present('livy-service', service_packages) }}
{% endif %}
