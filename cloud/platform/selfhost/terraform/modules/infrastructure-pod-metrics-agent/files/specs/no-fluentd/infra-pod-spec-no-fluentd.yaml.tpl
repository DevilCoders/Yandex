- apiVersion: v1
  kind: Pod
  metadata:
    name: api-infrastructure
    namespace: kube-system
    annotations:
      config_digest: ${infra_config_digest}
      scheduler.alpha.kubernetes.io/critical-pod: ""
    labels:
      role: api
  spec:
    priority: 2000000001
    hostNetwork: true
    hostPID: true
    priorityClassName: system-cluster-critical
    initContainers:
    - name: deploy-infra-configs
      image: registry.yandex.net/cloud/api/metadata:${metadata_version}
      command:
      - /usr/bin/metadata
      - --attribute-name
      - infra-configs
      imagePullPolicy: Always
      volumeMounts:
      - name: all-configs
        mountPath: /etc
        terminationGracePeriodSeconds: 30
    - name: set-fqdn-for-juggler-client
      image: registry.yandex.net/cloud/api/metadata:${metadata_version}
      volumeMounts:
      - name: all-configs
        mountPath: /etc
        terminationGracePeriodSeconds: 30
      command:
      - /usr/bin/jugglerclientfqdn
    containers:
    - name: juggler-client
      image: registry.yandex.net/cloud/platform/juggler-client:${juggler_version}
      volumeMounts:
      - name: juggler-bundle-manifest-file
        mountPath: /juggler-bundle/MANIFEST.json
        readOnly: true
      - name: platform-http-check-json
        mountPath: /juggler-bundle/platform-http-checks.json
        readOnly: true
      - name: juggler-client-config
        mountPath: /home/monitor/juggler/etc/client.conf
        readOnly: true
      - name: juggler-client-log
        mountPath: /var/log/juggler-client
        terminationGracePeriodSeconds: 30
      - name: var-log-fluent
        mountPath: /var/log/fluent
        terminationGracePeriodSeconds: 30
        readOnly: true
      - name: var-spool-push-client
        mountPath: /var/spool/push-client
        terminationGracePeriodSeconds: 30
        readOnly: true
      - name: push-client-config
        mountPath: /etc/yandex/statbox-push-client
        readOnly: true
      - name: hostfs-mon
        mountPath: /hostfs
        readOnly: true
    - name: metricsagent
      image: registry.yandex.net/cloud/api/metricsagent:608b125f7e
      imagePullPolicy: IfNotPresent
      resources:
        requests:
          cpu: 200m
          memory: ${metrics_agent_memory_limit}
        limits:
          memory: ${metrics_agent_memory_limit}
      volumeMounts:
      - name: metricsagent-config
        mountPath: /etc/metricsagent/metricsagent.yaml
        readOnly: true
      - name: var-log
        mountPath: /var/log
    - name: push-client
      image: registry.yandex.net/cloud/platform/push-client:${push_client_version}
      command:
      - /usr/bin/push-client
      - -c
      - /etc/yandex/statbox-push-client/push-client.yaml
      - -f
      - -w
      - --pid
      - /var/run/statbox/push-client.pid
      - --logger:mode=file
      - --logger:file=/var/log/fluent/watcher.log
      volumeMounts:
      - name: var-log
        mountPath: /var/log
        terminationGracePeriodSeconds: 30
      - name: var-spool
        mountPath: /var/spool
        terminationGracePeriodSeconds: 30
      - name: push-client-config
        mountPath: /etc/yandex/statbox-push-client
        readOnly: true
      - name: push-client-run
        mountPath: /var/run/statbox
    volumes:
    - name: all-configs
      hostPath:
       path: /etc
       type: DirectoryOrCreate
    - name: var-log
      hostPath:
        path: /var/log
    - name: var-spool
      hostPath:
        path: /var/spool
    - name: var-log-fluent
      hostPath:
        path: /var/log/fluent
    - name: var-spool-push-client
      hostPath:
        path: /var/spool/push-client
    - name: push-client-config
      hostPath:
        path: /etc/yandex/statbox-push-client
    - name: push-client-run
      hostPath:
        path: /var/run/statbox
        type: DirectoryOrCreate
    - name: etc-fluent
      hostPath:
        path: /etc/fluent
    - name: var-lib-docker-containers
      hostPath:
        path: /var/lib/docker/containers
    - name: var-log-journal
      hostPath:
        path: /run/log/journal
    - name: solomon-agent-config
      hostPath:
        path: /etc/solomon-agent
    - name: juggler-client-dir
      hostPath:
        path: /etc/api/configs/juggler-client
        type: DirectoryOrCreate
    - name: juggler-client-log
      hostPath:
        path: /var/log/juggler-client
        type: DirectoryOrCreate
    - name: juggler-bundle-manifest-file
      hostPath:
        path: /etc/api/configs/juggler-client/MANIFEST.json
        type: FileOrCreate
    - name: platform-http-check-json
      hostPath:
        path: /etc/api/configs/juggler-client/platform-http-check.json
        type: FileOrCreate
    - name: juggler-client-config
      hostPath:
        path: /etc/api/configs/juggler-client/juggler-client.conf
        type: FileOrCreate
    - name: hostfs-mon
      hostPath:
        path: /
    - name: metricsagent-config
      hostPath:
        path: /etc/metricsagent/metricsagent.yaml
