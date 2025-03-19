#!/bin/bash
# {% set url = salt.pillar.get("data:errata:url", "https://errata.s3.yandex.net/errata.json.gz") %}
# {% set skip = '*_mdb-bionic-secure' %}

{% if salt['pillar.get']('data:use_monrun', True) %}
{%     set user = 'monitor' %}
{%     set script = '/usr/local/yandex/monitoring/errata_updates.py' %}
{%     set monitoring_binary = 'monrun' %}
{% else %}
{%     set user = 'telegraf' %}
{%     set script = '/usr/local/yandex/telegraf/scripts/errata_updates.py' %}
{%     set monitoring_binary = 'mont' %}
{% endif %}

{% if not salt.dbaas.is_dataplane() %}
{% set warn_delay = salt.dbaas.errata_warn_threshold() %}
{% set warn_arg = '-w {0}'.format(warn_delay) %}
{% else %}
{% set warn_arg = '' %}
{% endif %}

if [ "$(sudo -u {{ user }} {{ script }} -u "{{ url }}" {{ warn_arg }})" != "PASSIVE-CHECK:errata_updates;0;OK" ]
then
    apt-get -qq update
    for package in $(sudo -u {{ user }} {{ script }} -u "{{ url }}" {{ warn_arg }} -v | grep -v grub | cut -d, -f1 | sort -u)
    do
        installed_version=$(dpkg -l | grep "\s$package\s" | awk '{print $3}')
        filename="$(apt-cache show "$package=$installed_version" 2>/dev/null | grep "^Filename:" | awk '{print $NF}')"
        if [ "$filename" != "" ]
        then
            # shellcheck disable=SC1083
            if grep -q "$filename" /var/lib/apt/lists/{{ skip }}_*_Packages
            then
                echo "Skipping $package (installed from {{ skip }})"
                continue
            fi
        fi
        if apt-mark showmanual | grep -q "^$package$"
        then
            apt-get -o Dpkg::Options::="--force-confold" -y --force-yes install "$package"
        else
            apt-get -o Dpkg::Options::="--force-confold" -y --force-yes install "$package"
            apt-mark auto "$package"
        fi
    done
    apt-get clean
    sudo -u {{ user }} {{ monitoring_binary }} -r errata_updates
fi
