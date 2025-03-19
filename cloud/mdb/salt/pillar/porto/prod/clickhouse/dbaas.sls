data:
    clickhouse:
        users:
            dbaas_writer:
                password: {{ salt.yav.get('ver-01dzb514qpbgvhqqtr5h9vfx4f[password]') }}
                hash: {{ salt.yav.get('ver-01dzb514qpbgvhqqtr5h9vfx4f[hash]') }}
                profile: default
                databases:
                    mdb: {}

            dbaas_reader:
                password: {{ salt.yav.get('ver-01dzb53mh0q9rfybyanqfr871c[password]') }}
                hash: {{ salt.yav.get('ver-01dzb53mh0q9rfybyanqfr871c[hash]') }}
                profile: readonly
                databases:
                    mdb: {}
