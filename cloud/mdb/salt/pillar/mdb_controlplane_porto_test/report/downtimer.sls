data:
    downtimer:
        root_group: mdb_dataplane_porto_test
        e2e_folder: dummy
        juggler:
            token: {{ salt.yav.get('ver-01g366s0xzbq0qr6vamev1wf3k[token]') }}
        sentry:
            dsn: {{ salt.yav.get('ver-01g0yfgf46zy4pc0qz9z40grkb[dsn]') }}
