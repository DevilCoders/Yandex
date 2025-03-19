data:
    config:
        salt:
            keys:
                pub: ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAD3H5KTc0x6mBoeiVmvlqbx9NHzeFog8BJB28lsQoH3xlxPnero+8rgYhg6jtM1MyPnJAJOsPWE1G82k63cEfQHhAFPy1BjnbpP0Hiv0URZSvbOvAXY3bsFi0fSwYPOWJQBRV2ZwosJnaoBc08ZBfh1XLtJ/b7N7omxaoEPKfkYndjz2A== root@salt01h.db.yandex.net-2018-03-21
            hash: {{ salt.yav.get('ver-01dwh59d6j0an6xj9jm9x77985[hash]') }}
            master:
                private: |
                    {{ salt.yav.get('ver-01dwh5d3bm27xa5gakgfjh30e0[private]') | indent(20) }}
                public: |
                    {{ salt.yav.get('ver-01dwh5d3bm27xa5gakgfjh30e0[public]') | indent(20) }}

