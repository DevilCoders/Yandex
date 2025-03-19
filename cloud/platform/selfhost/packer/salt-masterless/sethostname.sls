sethostname_usr_bin:
  file.managed:
  - name: /usr/bin/sethostname
  - mode: 0755
  - contents: |
      #!/usr/bin/env bash
      fqdn=$(curl --fail  -H "Metadata-Flavor: Google" -s http://169.254.169.254/computeMetadata/v1/instance/attributes/fqdn);
      if [ $? -eq 0 ]
      then
        /usr/bin/hostnamectl set-hostname "${fqdn}"
      else
        echo "no \"fqdn\" metadata key defined"
      fi

sethostname_systemd_unit:
  file.managed:
  - name: /lib/systemd/system/sethostname.service
  - contents: |
      [Unit]
      Description=Set Hostname as FQDN [CLOUD-41774]

      [Service]
      Type=oneshot
      ExecStart=/usr/bin/sethostname

      [Install]
      WantedBy=multi-user.target docker-compose.service

  service.enabled:
    - name: sethostname.service
