data:
    redis:
        repair_sentinel: False
        config:
            requirepass: '{{salt.yav.get('ver-01dvr5twethv8qgznncb5jkgw3[pass]')}}'
            masterauth: '{{salt.yav.get('ver-01dvr5twethv8qgznncb5jkgw3[pass]')}}'
            save: ''
            appendfsync: 'no'
            appendonly: 'no'
            maxmemory: 200MB
            maxmemory-policy: 'allkeys-lru'
redis-master: '{{ salt.grains.get('id') }}'
