data:
    config:
        salt:
            keys:
                pub2: {{ salt.yav.get('ver-01dwh530zs1haprmsgehgac75c[public]') }}
                priv: |
                    {{ salt.yav.get('ver-01dwh530zs1haprmsgehgac75c[private]') | indent(20) }}
            hash: {{ salt.yav.get('ver-01dwh59d6j0an6xj9jm9x77985[hash]') }}
            master:
                private: |
                    {{ salt.yav.get('ver-01dwh5d3bm27xa5gakgfjh30e0[private]') | indent(20) }}
                public: |
                    {{ salt.yav.get('ver-01dwh5d3bm27xa5gakgfjh30e0[public]') | indent(20) }}
