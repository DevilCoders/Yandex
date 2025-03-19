ingress-tst-wildcard-crt:
  kubernetes.secret_present:
    - kubeconfig: /etc/kubernetes/admin.conf
    - secret_type: kubernetes.io/tls
    - data:
        tls.crt: |
          {{ pillar['ingress-tst-wildcard-crt']['7F001E0B8BAA957F118D5428510002001E0B8B_certificate'] | indent(10) }}
        tls.key: |
          {{ pillar['ingress-tst-wildcard-crt']['7F001E0B8BAA957F118D5428510002001E0B8B_private_key'] | indent(10) }}
    - template: jinja
    - names:
      - ingress-tst-wildcard-default:
        - namespace: default
      - ingress-tst-wildcard-argocd:
        - namespace: argocd
      - ingress-tst-wildcard-kubernetes-dashboard:
        - namespace: kubernetes-dashboard
