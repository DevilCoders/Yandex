# Installing oslogin automatically sets datasource in /etc/cloud/cloud.cfg.d/91_gce.cfg
Set datasource to GCE:
  file.absent:
  - name: /etc/cloud/cloud.cfg.d/90_dpkg.cfg

Remove network configuration by cloud-init without dpkg-reconfigure:
  file.absent:
  - name: /etc/network/interfaces.d/50-cloud-init.cfg