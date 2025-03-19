Force using EC2 Metadata API even if the instance is running under cloud provider different from Amazon EC2:
  file.managed:
  - name: /etc/cloud/cloud.cfg.d/99-ec2-datasource.cfg
  - contents:
    - '#cloud-config'
    - 'datasource:'
    - ' Ec2:'
    - '  strict_id: false'

Add NoCloud Datasource to default datasources:
  file.managed:
  - name: /etc/cloud/cloud.cfg.d/90_dpkg.cfg
  - contents:
    - '# to update this file, run dpkg-reconfigure cloud-init'
    - 'datasource_list: [ NoCloud, Ec2 ]'

Disable network configuration by cloud-init:
  file.managed:
  - name: /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
  - contents:
    - 'network:'
    - ' config:'
    - '  disabled'

Adopt GCE DataSource for Yandex.Cloud:
  file.replace:
  - name: /usr/lib/python3/dist-packages/cloudinit/sources/DataSourceGCE.py
  - pattern: 'GoogleCloud-'
  - repl: 'YC-'
