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
      - name: hostfs-mon
        mountPath: /hostfs
        readOnly: true
    - name: logcleaner
      image: registry.yandex.net/cloud/api/logcleaner:${logcleaner_version}
      command: ["/usr/bin/logcleaner", "--num-files-to-keep", "${logcleaner_files_to_keep}"]
      volumeMounts:
      - name: var-log
        mountPath: /var/log
        terminationGracePeriodSeconds: 30
    - name: fluent
      image: registry.yandex.net/cloud/api/fluent:${fluent_version}
      imagePullPolicy: Always
      env:
      - name: FLUENT_UID
        value: "0"
      - name: FLUENTD_OPT
        value: "--log-rotate-age 5 --log-rotate-size 104857600"
      volumeMounts:
      - name: var-log
        mountPath: /var/log
        terminationGracePeriodSeconds: 30
      - name: etc-fluent
        mountPath: /fluentd/etc
        readOnly: true
      - name: etc-fluent
        mountPath: /etc/fluent
        readOnly: true
      - name: var-lib-docker-containers
        mountPath: /var/lib/docker/containers
        readOnly: true
      - name: var-log-journal
        mountPath: /var/log/journal
        readOnly: true
    volumes:
    - name: all-configs
      hostPath:
       path: /etc
       type: DirectoryOrCreate
    - name: var-log
      hostPath:
        path: /var/log
    - name: etc-fluent
      hostPath:
        path: /etc/fluent
    - name: var-lib-docker-containers
      hostPath:
        path: /var/lib/docker/containers
    - name: var-log-journal
      hostPath:
        path: /run/log/journal
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
