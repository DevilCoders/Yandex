{% set unit="ipxe_check" %}

/usr/local/bin/bootutil64e:
  file.managed:
    - source: salt://files/{{unit}}/bootutil64e
    - user: root
    - group: root
    - mode: 755

ipxe_check:
  file.managed:
    - name: /usr/local/bin/monrun_ipxe_check.sh
    - source: salt://files/{{unit}}/monrun_ipxe_check.sh
    - user: root
    - group: root
    - mode: 755

  monrun.present:
    - command: "sudo /usr/local/bin/monrun_ipxe_check.sh"
    - execution_interval: 600
    - execution_timeout: 20
    - type: common
