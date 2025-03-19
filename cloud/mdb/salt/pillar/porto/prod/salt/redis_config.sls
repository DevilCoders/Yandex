data:
    redis:
        repair_sentinel: False
        config:
            requirepass: '{{ salt.yav.get('ver-01dvx4gg50x5g7d4taywzjy365[pass]') }}'
            masterauth: '{{ salt.yav.get('ver-01dvx4gg50x5g7d4taywzjy365[pass]') }}'
            save: ''
            appendfsync: 'no'
            appendonly: 'no'
            maxmemory: 200MB
            maxmemory-policy: 'allkeys-lru'
