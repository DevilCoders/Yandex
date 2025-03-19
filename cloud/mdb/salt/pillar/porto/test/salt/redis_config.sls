data:
    redis:
        repair_sentinel: False
        config:
            requirepass: '{{ salt.yav.get('ver-01dthb74ww398kq151gddp5sgj[pass]') }}'
            masterauth: '{{ salt.yav.get('ver-01dthb74ww398kq151gddp5sgj[pass]') }}'
            save: ''
            appendfsync: 'no'
            appendonly: 'no'
            maxmemory: 200MB
            maxmemory-policy: 'allkeys-lru'
redis-master: '{{ salt.grains.get('id') }}'
