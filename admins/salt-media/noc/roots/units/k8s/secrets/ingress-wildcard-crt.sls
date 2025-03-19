ingress-wildcard-crt:
  kubernetes.secret_present:
    - kubeconfig: /etc/kubernetes/admin.conf
    - secret_type: kubernetes.io/tls
    - data:
        tls.crt: |
          {{ pillar['ingress-wildcard-crt']['7F001D852A460576BE869EF4DF0002001D852A_certificate'] | indent(10) }}
        tls.key: |
          {{ pillar['ingress-wildcard-crt']['7F001D852A460576BE869EF4DF0002001D852A_private_key'] | indent(10) }}
    - template: jinja
    - names:
      - ingress-wildcard-default:
        - namespace: default
      - ingress-wildcard-argocd:
        - namespace: argocd
      - ingress-wildcard-kubernetes-dashboard:
        - namespace: kubernetes-dashboard
