repo:
  pkgrepo.managed:
    - humanname: "Latest stable lxd"
    - name: deb http://ppa.launchpad.net/ubuntu-lxc/lxd-stable/ubuntu {{ salt['grains.get']('oscodename', 'xenial') }} main
    - file: /etc/apt/sources.list.d/lxd.list
    - keyid: 7635B973
    - keyserver: keys.openpgp.org
