{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt.pillar.get('gpdb_master') or salt.grains.get('greenplum:role') == 'master' %}
gpdb_sync_resource_groups:
  mdb_greenplum.sync_resource_groups:
    - rg_names:
      - mdb_admin_group

gpdb_sync_users:
  mdb_greenplum.sync_users:
    - require:
      - mdb_greenplum: gpdb_sync_resource_groups
{% endif %}
