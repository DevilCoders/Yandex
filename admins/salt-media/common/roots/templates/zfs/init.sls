remove-generic:
  cmd.run:
    - name: dpkg-query -l 'linux-image*-generic' | awk {' print $2 '} | grep linux-image | xargs apt-get purge -y
    - unless: dpkg-query --show '--showformat=${Status}' zfs-dkms | grep 'install ok installed'

headers:
  pkg.installed:
    - pkgs:
      - linux-headers-{{ grains["kernelrelease"] }}

zfs:
  pkg.installed:
    - pkgs:
      - spl
      - spl-dkms
      - zfsutils-linux
      - zfs-dkms
    - required:
      - headers
      - remove-generic
