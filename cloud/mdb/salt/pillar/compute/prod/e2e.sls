data:
    dbaas-e2e:
        oauth_token: {{ salt.yav.get('ver-01e2jzjthg1se3w9qxyvm75g31[token]') }}
        ssh_public_key: 'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINlwyXBdLwT6XMJnE0CfUc0qOm6M/NUFWK3t45yumkrd'
        ssh_private_key: |
            {{ salt.yav.get('ver-01e2mtdk9xxtk37d94ad4vp5sz[key]') | indent(12) }}
