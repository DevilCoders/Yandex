data:
    walg:
        ssh_keys:
            private: |
                {{ salt.yav.get('ver-01ejxyyntv69c22qfmzyqj3dkk[private]') | indent(16) }}
            pub: {{ salt.yav.get('ver-01ejxyyntv69c22qfmzyqj3dkk[public]') }}
