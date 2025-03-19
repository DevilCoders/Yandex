data:
    clickhouse:
        users:
            dbaas_writer:
                password: {{ salt.yav.get('ver-01dweta172yyz3q8k602fqymry[password]') }}
                hash: {{ salt.yav.get('ver-01dweta172yyz3q8k602fqymry[hash]') }}
                databases:
                    - mdb

            dbaas_reader:
                password: {{ salt.yav.get('ver-01dwetc2kh5d4cj9cwz8z9cynb[password]') }}
                hash: {{ salt.yav.get('ver-01dwetc2kh5d4cj9cwz8z9cynb[hash]') }}
                profile: readonly
                databases:
                    - mdb
