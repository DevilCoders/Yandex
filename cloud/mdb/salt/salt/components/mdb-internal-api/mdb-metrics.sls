{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
include:
    - components.mdb-metrics

{{ deploy_configs('mdb-internal-api') }}

yasmagent-instance-getter:
    file.absent:
        - name: /eiuhgeruigh4385ytg48gh4
