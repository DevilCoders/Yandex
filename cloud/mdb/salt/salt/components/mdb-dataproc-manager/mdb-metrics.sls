{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}

{% if salt.grains.get('id') == "dataproc-manager-preprod01f.cloud-preprod.yandex.net" or salt.grains.get('id') == "dataproc-manager01f.yandexcloud.net" %}
{{ deploy_configs('dataproc-manager') }}
{% endif %}
