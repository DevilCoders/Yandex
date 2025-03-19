{% set knox_bin_path = '/usr/lib/knox/bin' %}

{% import 'components/hadoop/macro.sls' as m with context %}

{% set packages = {
    'masternode': {
        'knox': m.version('knox')
    }
}%}

{{ m.pkg_present('knox', packages) }}

