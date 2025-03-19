data:
    docker_private_config:
        auths:
            registry.yandex.net:
                auth: {{ salt.yav.get('ver-01e0qf8g2af4qfekvka2vn0e2f[token]') }}
