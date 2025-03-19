yc cli package:
  pkg.installed:
    - pkgs:
      - yc-cli

yc installation:
  cmd.run:
    - name: "curl https://storage.yandexcloud.net/yandexcloud-yc/install.sh | bash -s -- -i /opt/yandexcloud-cli/ -r /etc/profile"
    - creates: /opt/yandexcloud-cli/bin/yc