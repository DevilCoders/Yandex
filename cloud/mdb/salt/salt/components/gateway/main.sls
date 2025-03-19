dataproc-manager-hostname:
    host.present:
        - ip: 127.0.0.1
        - names:
            - {{ salt['pillar.get']('data:gateway:cert_server_name') }}

/opt/yandex/gateway/bin/wait_grpc.sh:
    file.managed:
        - source: salt://{{ slspath }}/wait_grpc.sh
        - mode: 755
        - makedirs: True

/opt/yandex/gateway/etc/configserver.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/configserver.yaml
        - makedirs: True

/opt/yandex/gateway/etc/gateway-services.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/gateway-services.yaml
        - template: jinja
        - makedirs: True

/opt/yandex/gateway/etc/gateway.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/gateway.yaml
        - makedirs: True

/opt/yandex/gateway/etc/envoy-resources.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/envoy-resources.yaml
        - makedirs: True

/opt/yandex/gateway/etc/envoy.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/envoy.yaml
        - makedirs: True

/opt/yandex/gateway/etc/adapter.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/adapter.yaml
        - template: jinja
        - makedirs: True

/opt/yandex/gateway/etc/ssl:
    file.directory:
        - mode: 755
        - makedirs: True

/var/log/envoy:
    file.directory:
        - mode: 755
        - makedirs: True

/opt/yandex/gateway/etc/ssl/api-gateway.crt:
    file.managed:
        - contents_pillar: data:infratest_cert
        - require:
            - file: /opt/yandex/gateway/etc/ssl

/opt/yandex/gateway/etc/ssl/api-gateway.key:
    file.managed:
        - contents_pillar: data:infratest_cert_key
        - require:
            - file: /opt/yandex/gateway/etc/ssl

gateway-images:
    docker_image.present:
        - names:
            - cr.yandex/crp7nvlkttssi7kapoho/api/configserver:a12155fba1
            - cr.yandex/crp7nvlkttssi7kapoho/api/gateway:a12155fba1
            - cr.yandex/crpjc8hshkc404hj1tua/cloud-api-adapter-application:24895-f4b9a30c65
            - cr.yandex/crp7nvlkttssi7kapoho/envoy:v1.17.3-20-gc855abe1a
        - require:
            - pkg: docker-pkgs

{% if salt['pillar.get']('data:gateway:is_cache', False) == False %}
/etc/hostname:
    file.managed:
        - contents: {{ salt['grains.get']('id') }}

set-hostname:
    cmd.wait:
        - name: hostname {{ salt['grains.get']('id') }}
        - watch:
            - file: /etc/hostname
        - order: 1
        - require_in:
            - service: redis-is-running

configserver-container:
    docker_container.running:
        - name: configserver
        - image: cr.yandex/crp7nvlkttssi7kapoho/api/configserver:a12155fba1
        - auto_remove: True
        - binds: /opt/yandex/gateway/etc/:/etc/configserver/
        - network_mode: "host"
        - require:
            - pkg: docker-pkgs
            - docker_image: gateway-images
            - file: /opt/yandex/gateway/etc/configserver.yaml
            - file: /opt/yandex/gateway/etc/gateway-services.yaml
            - cmd: set-hostname
        - watch:
            - file: /opt/yandex/gateway/etc/configserver.yaml
            - file: /opt/yandex/gateway/etc/gateway-services.yaml

configserver-wait:
    cmd.run:
        - name: /opt/yandex/gateway/bin/wait_grpc.sh
        - env:
          - PORT: 4435
        - require:
            - docker_container: configserver-container

gateway-container:
    docker_container.running:
        - name: gateway
        - image: cr.yandex/crp7nvlkttssi7kapoho/api/gateway:a12155fba1
        - auto_remove: True
        - binds: /opt/yandex/gateway/etc/:/etc/gateway/
        - network_mode: "host"
        - require:
            - pkg: docker-pkgs
            - file: /opt/yandex/gateway/etc/gateway-services.yaml
            - file: /opt/yandex/gateway/etc/gateway.yaml
            - docker_container: configserver
            - cmd: configserver-wait
            - docker_image: gateway-images
            - cmd: set-hostname
        - watch:
            - file: /opt/yandex/gateway/etc/gateway-services.yaml
            - file: /opt/yandex/gateway/etc/gateway.yaml
            - docker_container: configserver

gateway-wait:
    cmd.run:
        - name: /opt/yandex/gateway/bin/wait_grpc.sh
        - env:
            - PORT: 9894
        - require:
            - docker_container: gateway-container

envoy-container:
    docker_container.running:
        - name: envoy
        - image: cr.yandex/crp7nvlkttssi7kapoho/envoy:v1.17.3-20-gc855abe1a
        - binds:
            - /opt/yandex/gateway/etc/: /etc/envoy/
            - /var/log/envoy/: /var/log/envoy/
        - auto_remove: True
        - network_mode: "host"
        - require:
            - pkg: docker-pkgs
            - file: /opt/yandex/gateway/etc/envoy-resources.yaml
            - file: /opt/yandex/gateway/etc/envoy.yaml
            - file: /var/log/envoy
            - docker_container: configserver
            - docker_container: gateway
            - docker_container: adapter
            - cmd: configserver-wait
            - cmd: gateway-wait
            - cmd: adapter-wait
            - cmd: set-hostname
        - watch:
            - file: /opt/yandex/gateway/etc/envoy-resources.yaml
            - file: /opt/yandex/gateway/etc/envoy.yaml
            - docker_container: configserver
            - docker_container: gateway
            - docker_container: adapter

adapter-container:
    docker_container.running:
        - name: adapter
        - environment:
            - APPLICATION_YAML: /etc/adapter/adapter.yaml
        - image: cr.yandex/crpjc8hshkc404hj1tua/cloud-api-adapter-application:24895-f4b9a30c65
        - auto_remove: True
        - binds: /opt/yandex/gateway/etc/:/etc/adapter/
        - network_mode: "host"
        - require:
            - pkg: docker-pkgs
            - file: /opt/yandex/gateway/etc/adapter.yaml
            - docker_container: configserver
            - docker_image: gateway-images
            - cmd: set-hostname
        - watch:
            - file: /opt/yandex/gateway/etc/adapter.yaml
            - docker_container: configserver

adapter-wait:
    cmd.run:
        - name: /opt/yandex/gateway/bin/wait_grpc.sh
        - env:
            - PORT: 6666
        - require:
            - docker_container: adapter-container
{% endif %}
