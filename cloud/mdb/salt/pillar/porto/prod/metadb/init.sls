include:
  - porto.prod.metadb.default_pillar
  - porto.prod.metadb.config_host_access_ids
  - porto.prod.metadb.feature_flags
  - porto.prod.metadb.regions
  - porto.prod.metadb.geos
  - porto.prod.metadb.disk_types
  - porto.prod.metadb.flavor_type
  - porto.prod.metadb.flavors
  - porto.prod.metadb.valid_resources
  - porto.prod.metadb.cluster_type_pillars
  - porto.prod.metadb.role_pillars
  - metadb_default_versions
  - metadb_default_alert

data:
  dbaas_metadb:
    cleaner_token: {{ salt.yav.get('ver-01e3nfakreja0rbjvxtpbm5wzc[token]') }}
    internal_api_url: api.db.yandex-team.ru
    delete_strategy: delete
    cleaner:
        service_account:
            id: {{ salt.yav.get('ver-01er1mz2yqs5fejcz04vq4e0bb[id]') }}
            key_id: {{ salt.yav.get('ver-01er1mz2yqs5fejcz04vq4e0bb[key_id]') }}
            private_key: {{ salt.yav.get('ver-01er1mz2yqs5fejcz04vq4e0bb[private_key]') | yaml_encode }}
