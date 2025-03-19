data:
    dbaas-e2e:
        oauth_token: {{ salt.yav.get('ver-01dws506nkwwxa3e9tdsfms63j[token]') }}
        ssh_public_key: 'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIHQZ8u6z42FXTQGRSMJrAZph8jHc3q2IG/pU8CHKR4xn'
        ssh_private_key: |
            {{ salt.yav.get('ver-01e2dxjvtb24tzq4r3748npwmh[key]') | indent(12) }}
