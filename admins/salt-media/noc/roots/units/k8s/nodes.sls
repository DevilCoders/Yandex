{%set dc = grains["conductor"]["root_datacenter"] if grains['yandex-environment'] == 'production' else 'test'%}

include:
  - units.k8s.main-packages

Yandex HBF Agent:
  pkg.installed:
    - pkgs:
      - yandex-hbf-agent-static
      - yandex-hbf-agent-init

/usr/share/yandex-hbf-agent/rules.d/04-k8s-exceptions.v6:
  file.managed:
    - source: salt://{{ slspath }}/files/yandex-hbf-agent/rules.d/04-k8s-exceptions.v6
/usr/share/yandex-hbf-agent/rules.d/04-k8s-exceptions.v4:
  file.managed:
    - source: salt://{{ slspath }}/files/yandex-hbf-agent/rules.d/04-k8s-exceptions.v4

/root/.bashrc:
  file.append:
    - text:
      - "export KUBECONFIG=/etc/kubernetes/kubelet.conf"
      - "source <(kubectl completion bash)"

/etc/modules-load.d/03-lxd-br-netfilter.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        br_netfilter
/etc/modules-load.d/03-k8s.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        overlay
# kube proxy try to change hashsize parameter of nf_conntrack if it not equil 2097152
# https://www.claudiokuenzler.com/blog/1106/unable-to-deploy-rancher-managed-kubernetes-cluster-lxc-lxd-nodes
/etc/modprobe.d/k8s-conntrack.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        options nf_conntrack hashsize=2097152

'kubeadm join {{pillar["api-hostname"]}}:6443 --token {{pillar["sec"]["token-"+dc]}} --discovery-token-ca-cert-hash {{pillar["sec"]["ca-cert-hash-"+dc]}}':
  cmd.run:
    - creates: /etc/kubernetes/pki/ca.crt
