{% from "components/mdb-metrics/lib.sls" import yasmgetter_config with context %}
include:
    - components.mdb-metrics

{{ yasmgetter_config('mdb-deploy-api-yasmagent-instance-getter') }}
