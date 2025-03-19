include:
  - compute.preprod.metadb.default_pillar
  - compute.preprod.metadb.feature_flags
  - compute.preprod.metadb.regions
  - compute.preprod.metadb.geos
  - compute.preprod.metadb.disk_types
  - compute.preprod.metadb.flavor_type
  - compute.preprod.metadb.flavors
  - compute.preprod.metadb.valid_resources
  - compute.preprod.metadb.cluster_type_pillars
  - compute.preprod.metadb.role_pillars
  - compute.preprod.metadb.config_host_access_ids
  - metadb_default_versions
  - metadb_default_alert

data:
  dbaas_metadb:
    cleaner_folder: 'null'
    cleaner_token: {{ salt.yav.get('ver-01e8m07z8nz6d9xk01fbdzenj2[token]') }}
    internal_api_url: api-admin-dbaas-preprod01k.cloud-preprod.yandex.net
    delete_strategy: stop-delete
    cleaner_label_key: mdb-auto-purge
    cleaner_label_value: "'off'"
    cleaner_max_age: '7 days'
    iam_url: identity.private-api.cloud-preprod.yandex.net:14336
    cleaner:
        service_account:
            id: {{ salt.yav.get('ver-01er1n45jw489qfnv0yq6mps31[id]') }}
            key_id: {{ salt.yav.get('ver-01er1n45jw489qfnv0yq6mps31[key_id]') }}
            private_key: {{ salt.yav.get('ver-01er1n45jw489qfnv0yq6mps31[private_key]') | yaml_encode }}
