data:
    redis:
        repair_sentinel: False
        config:
            requirepass: '{{ salt.yav.get('ver-01dwca9xy3p1mf1mq1m7areqsa[pass]') }}'
            masterauth: '{{ salt.yav.get('ver-01dwca9xy3p1mf1mq1m7areqsa[pass]') }}'
            save: ''
            appendfsync: 'no'
            appendonly: 'no'
            maxmemory: 200MB
            maxmemory-policy: 'allkeys-lru'
