{# Remove these states after CLOUD-21976 has been deployed to compute #}
cleanup_old_monrun_files:
  file.absent:
    - names:
      - /etc/monrun/conf.d/e2e-tests-permnet-dns.conf
      - /etc/monrun/conf.d/e2e-tests-permnet-connectivity.conf

{# Make sure juggler-client does not run the check after we delete it #}
/usr/bin/monrun --gen-jobs:
  cmd.run:
    - onchanges:
      - file: cleanup_old_monrun_files

{%- import_yaml slspath + "/monitoring.yaml" as monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
