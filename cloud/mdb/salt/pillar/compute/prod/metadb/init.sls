include:
  - compute.prod.metadb.default_pillar
  - compute.prod.metadb.feature_flags
  - compute.prod.metadb.regions
  - compute.prod.metadb.geos
  - compute.prod.metadb.disk_types
  - compute.prod.metadb.flavor_type
  - compute.prod.metadb.flavors
  - compute.prod.metadb.valid_resources
  - compute.prod.metadb.cluster_type_pillars
  - compute.prod.metadb.role_pillars
  - compute.prod.metadb.config_host_access_ids
  - metadb_default_versions
  - metadb_default_alert

data:
  dbaas_metadb:
    enable_cleaner: False
