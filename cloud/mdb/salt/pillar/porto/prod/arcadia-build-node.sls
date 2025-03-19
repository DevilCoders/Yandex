data:
    arcadia-build-node:
        robot-pgaas-ci.ya_token: {{ salt.yav.get('ver-01e2dj26n77x7ne1bedp3n1z1a[oauth]') }}
        robot-pgaas-ci.key: |
            {{ salt.yav.get('ver-01e0qe75zqn1hagcd8pe4tmakm[public]') | indent(12) }}
