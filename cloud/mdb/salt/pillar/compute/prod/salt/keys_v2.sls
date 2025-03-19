data:
    config:
        salt:
            keys:
                pub2: {{ salt.yav.get('ver-01e95hpszvwnkpdesnbzzcz2e6[public]') }}
                priv: |
                    {{ salt.yav.get('ver-01e95hpszvwnkpdesnbzzcz2e6[private]') | indent(20) }}
            hash: {{ salt.yav.get('ver-01e95hx4de9g8mr48q1ktqj59q[hash]') }}
            master:
                private: |
                    {{ salt.yav.get('ver-01e95gxzpjd6w2ywhxxqmevqn5[private]') | indent(20) }}
                public: |
                    {{ salt.yav.get('ver-01e95gxzpjd6w2ywhxxqmevqn5[public]') | indent(20) }}
