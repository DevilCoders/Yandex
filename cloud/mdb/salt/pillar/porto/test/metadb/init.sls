include:
  - porto.test.metadb.default_pillar
  - porto.test.metadb.config_host_access_ids
  - porto.test.metadb.feature_flags
  - porto.test.metadb.regions
  - porto.test.metadb.geos
  - porto.test.metadb.disk_types
  - porto.test.metadb.flavor_type
  - porto.test.metadb.flavors
  - porto.test.metadb.valid_resources
  - porto.test.metadb.cluster_type_pillars
  - porto.test.metadb.role_pillars
  - metadb_default_versions
  - metadb_default_alert

data:
  dbaas_metadb:
    cleaner_token: {{ salt.yav.get('ver-01e9dz2qtejcdptp83csffwnc4[token]') }}
    internal_api_url: internal-api-test.db.yandex-team.ru
    cleaner_folder: 'null'
    cleaner_label_key: mdb-auto-purge
    cleaner_label_value: "'off'"
    delete_strategy: delete
    cleaner:
        service_account:
            id: {{ salt.yav.get('ver-01er1mz2yqs5fejcz04vq4e0bb[id]') }}
            key_id: {{ salt.yav.get('ver-01er1mz2yqs5fejcz04vq4e0bb[key_id]') }}
            private_key: {{ salt.yav.get('ver-01er1mz2yqs5fejcz04vq4e0bb[private_key]') | yaml_encode }}
