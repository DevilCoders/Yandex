{% set init_tsm_binary = salt['cmd.shell']('find /opt/tableau/tableau_server/packages/ -name initialize-tsm') %}

tableau-initialize:
   cmd.run:
     - name: {{ init_tsm_binary }} --accepteula -a {{ salt['pillar.get']('admin_user') }}

{% set tsm_binary  = salt['cmd.shell']('find /opt/tableau/tableau_server/packages/bin* -name tsm') %}

{% set tabcmd_binary  = salt['cmd.shell']('find /opt/tableau/tableau_server/packages/bin* -name tabcmd') %}

tableau-configure-identity-store:
   cmd.run:
     - name: "{{ tsm_binary }} settings import -f ./config.json"
     - cwd: /tmp/tableau

tableau-apply-changes:
   cmd.run:
     - name: "{{ tsm_binary }} pending-changes apply"

tableau-activate-trial-license:
   cmd.run:
     - name: "{{ tsm_binary }} licenses activate -t"

tableau-register:
   cmd.run:
     - name: "{{ tsm_binary }} register --file  ./registration.json"
     - cwd: /tmp/tableau

tableau-apply-changes-registration:
   cmd.run:
     - name: "{{ tsm_binary }} pending-changes apply"

tableau-start-server:
   cmd.run:
     - name: "{{ tsm_binary }} initialize --start-server --request-timeout 1800"

tableau-create-admin-user:
   cmd.run:
     - name: "{{ tabcmd_binary }} initialuser --server 'localhost:443' --username {{ salt['pillar.get']('admin_user') }}  --password {{ salt['pillar.get']('admin_user_password') }}"

tableau-metadata-services-enable:
   cmd.run:
     - name: "{{ tsm_binary }} maintenance metadata-services enable --ignore-prompt"
