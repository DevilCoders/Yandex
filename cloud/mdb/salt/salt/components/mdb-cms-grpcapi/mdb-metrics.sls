{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
include:
    - components.mdb-metrics

{{ deploy_configs('mdb-cms-instance-api') }}
