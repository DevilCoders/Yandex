data:
    config:
        salt:
            keys:
                pub2: |
                    ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAD3H5KTc0x6mBoeiVmvlqbx9NHzeFog8BJB28lsQoH3xlxPnero+8rgYhg6jtM1MyPnJAJOsPWE1G82k63cEfQHhAFPy1BjnbpP0Hiv0URZSvbOvAXY3bsFi0fSwYPOWJQBRV2ZwosJnaoBc08ZBfh1XLtJ/b7N7omxaoEPKfkYndjz2A== root@salt01h.db.yandex.net-2018-03-21
                    ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAHJPZfLQgj58Xst5zB0ehQWt0o6BYSntOmBsvsGeN12M/+u6F2wdeUCMevH/oosxZ/5VdDdVexVpVAK3dunX5Z9NQG0Rt9kAijMgA+e3LBnvX19yZmr1uts0gt9MCyrWA1raG2++fA5/wKz57bbW3+YVrp0ss6/tZahpu05loqpM/DWaw== root@salt_deploy_v2
                priv: |
                    {{ salt.yav.get('ver-01dtv8mw0dbe67z8jvvdrhcy70[private]') | indent(20) }}
            hash: {{ salt.yav.get('ver-01dtv5zxew0nwvwcsybertp1h6[hash]') }}
            master:
                private: |
                    {{ salt.yav.get('ver-01dvchg68zhj4jeep09xwqbmaw[private]') | indent(20) }}
                public: |
                    {{ salt.yav.get('ver-01dvchg68zhj4jeep09xwqbmaw[public]') | indent(20) }}
