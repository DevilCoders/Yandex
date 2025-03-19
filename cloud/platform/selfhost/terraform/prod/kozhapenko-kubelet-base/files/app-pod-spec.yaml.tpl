- apiVersion: v1
  kind: Pod
  metadata:
    name: api-static
    namespace: kube-system
    labels:
      role: api
    annotations:
      config_digest: ${config_digest}
      scheduler.alpha.kubernetes.io/critical-pod: ""
  spec:
    priority: 2000000001
    priorityClassName: system-cluster-critical
    hostNetwork: true
    initContainers:
    - name: deploy-configs
      image: registry.yandex.net/cloud/api/metadata:1b931b4a1
      volumeMounts:
      - name: all-configs
        mountPath: /etc
        terminationGracePeriodSeconds: 30
    containers:
    - name: api-configserver
      image: registry.yandex.net/cloud/api/configserver:92afe9515
      volumeMounts:
      - name: configserver-logs
        mountPath: /var/log/configserver
        terminationGracePeriodSeconds: 30
      - name: configserver-configs
        mountPath: /etc/configserver
        readOnly: true
      - name: etc-passwd
        mountPath: /etc/passwd
        readOnly: true
    - name: statsd-exporter
      image: registry.yandex.net/cloud/api/statsd-exporter:v0.6.0
    - name: jaeger-agent
      image: jaegertracing/jaeger-agent:1.12.0
      args:
        - --reporter.grpc.host-port=jaeger-collector.private-api.ycp.cloud.yandex.net:443
        - --reporter.grpc.tls
        - --reporter.grpc.tls.ca=/etc/envoy/ssl/certs/allCAs.pem
        - --reporter.grpc.retry.max=100
        - --log-level=debug
      volumeMounts:
        - mountPath: /etc/envoy/ssl/certs/
          name: envoy-certs
          readOnly: true
    - name: api-als
      image: registry.yandex.net/cloud/api/als:874808c65
      volumeMounts:
      - name: als-logs
        mountPath: /var/log/als
        terminationGracePeriodSeconds: 30
      - name: als-envoy-logs
        mountPath: /var/log/envoy
        terminationGracePeriodSeconds: 30
      - name: als-configs
        mountPath: /etc/als
        readOnly: true
      - name: etc-passwd
        mountPath: /etc/passwd
        readOnly: true
    - name: api-gateway
      image: registry.yandex.net/cloud/api/gateway:92afe9515
      volumeMounts:
      - name: gateway-logs
        mountPath: /var/log/gateway
        terminationGracePeriodSeconds: 30
      - name: gateway-configs
        mountPath: /etc/gateway
        readOnly: true
      - name: etc-passwd
        mountPath: /etc/passwd
        readOnly: true
    - name: api-envoy
      image: registry.yandex.net/cloud/api/envoy:v1.11.1-12-g01fe140
      volumeMounts:
      - name: envoy-logs
        mountPath: /var/log/envoy
        terminationGracePeriodSeconds: 30
      - name: envoy-configs
        mountPath: /etc/envoy
        readOnly: true
    volumes:
    - name: configserver-logs
      hostPath:
        path: /var/log/configserver
        type: DirectoryOrCreate
    - name: configserver-configs
      hostPath:
        path: /etc/api/configs/configserver
        type: DirectoryOrCreate
    - name: als-logs
      hostPath:
        path: /var/log/als
        type: DirectoryOrCreate
    - name: als-envoy-logs
      hostPath:
        path: /var/log/envoy
        type: DirectoryOrCreate
    - name: als-configs
      hostPath:
        path: /etc/api/configs/als
        type: DirectoryOrCreate
    - name: envoy-logs
      hostPath:
        path: /var/log/envoy
        type: DirectoryOrCreate
    - name: envoy-configs
      hostPath:
        path: /etc/api/configs/envoy
        type: DirectoryOrCreate
    - name: gateway-logs
      hostPath:
        path: /var/log/gateway
        type: DirectoryOrCreate
    - name: gateway-configs
      hostPath:
        path: /etc/api/configs/gateway
        type: DirectoryOrCreate
    - name: all-configs
      hostPath:
        path: /etc
        type: DirectoryOrCreate
    - name: etc-passwd
      hostPath:
        path: /etc/passwd
    - name: envoy-certs
      hostPath:
        path: /etc/api/configs/envoy/ssl/certs/
        type: DirectoryOrCreate
