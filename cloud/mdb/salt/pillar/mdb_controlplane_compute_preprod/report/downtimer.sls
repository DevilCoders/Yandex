data:
    downtimer:
        root_group: mdb_dataplane_compute_preprod
        e2e_folder: aoed5i52uquf5jio0oec
        juggler:
            token: {{ salt.yav.get('ver-01g366sb0w6hdg76gh40f5sm3x[token]') }}
        sentry:
            dsn: {{ salt.yav.get('ver-01f8py42hbyvebhkjwfx5frgfn[dsn]') }}
