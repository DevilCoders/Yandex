data:
    kvm_host:
        template:
            api_url: 'http://dogfood.cloud.yandex.net'
            oauth_token: {{ salt.yav.get('ver-01e0q7m9kb7sd25b3vy6f4bx2k[token]') }}
            organization: '50000000-0000-0000-0000-000000000000'
            project: '00000000-0000-0000-0000-000000000000'
            pub: |
                {{ salt.yav.get('ver-01e0q7jfffdn5ccwqx4d7qn9z2[public]') | indent(16) }}
            pem: |
                {{ salt.yav.get('ver-01e0q7jfffdn5ccwqx4d7qn9z2[private]') | indent(16) }}
            master_pub: |
                {{ salt.yav.get('ver-01dz8m1pzmh1tmw8s75mygdvnf[public]') | indent(16) }}
