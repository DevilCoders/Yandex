yc:
  cmd.run:
    - name: "curl https://storage.yandexcloud.net/yandexcloud-yc/install.sh | bash -s -- -n -i /usr/"    

kubectl:
  cmd.run:
    - name: curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && curl -LO "https://dl.k8s.io/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl.sha256" && echo "$(cat kubectl.sha256)  kubectl" | sha256sum --check && sudo install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl

helm:
  cmd.run:
    - name: curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash


update_bashrc_root:
  file.managed:
    - name: '/root/.bashrc'
    - source: salt://{{ slspath }}/files/bashrc_skel.sh
    - user: root
    - group: root
    - mode: 0644

update_bashrc_skel:
  file.managed:
    - name: '/etc/skel/.bashrc'
    - source: salt://{{ slspath }}/files/bashrc_skel.sh
    - user: root
    - group: root
    - mode: 0644

kubectl-init:
  file.managed:
    - name: '/usr/bin/kubectl-init.sh'
    - source: salt://{{ slspath }}/files/kubectl-init.sh
    - user: root
    - group: root
    - mode: 0755
