apiVersion: v1
kind: Pod
metadata:
  name: yc-api-adapter
spec:
  initContainers:
  - name: copy-config-and-secrets
    image: cosmintitei/bash-curl
    command: ['sh', '-c', 'for meta in $(curl -H "Metadata-Flavor:Google" http://169.254.169.254/computeMetadata/v1/instance/attributes/export_files -s); do curl -H "Metadata-Flavor:Google" http://169.254.169.254/computeMetadata/v1/instance/attributes/$meta -s > /etc/yc/yc-api-adapter/$meta; done']
    volumeMounts:
    - name: yc-api-adapter-configs
      mountPath: /etc/yc/yc-api-adapter
  containers:
  - name: yc-api-adapter
    image: registry.yandex.net/cloud/${image}
    env:
    - name: APPLICATION_YAML
      value: /etc/yc/yc-api-adapter/config
    volumeMounts:
    - name: yc-api-adapter-logs
      mountPath: /var/log/yc-api-adapter
      terminationGracePeriodSeconds: 30
    - name: yc-api-adapter-configs
      mountPath: /etc/yc/yc-api-adapter
      readOnly: true
  hostNetwork: true
  volumes:
  - name: yc-api-adapter-logs
    hostPath:
      path: /var/log/yc-api-adapter
  - name: yc-api-adapter-configs
    hostPath:
      path: /etc/yc/yc-api-adapter
