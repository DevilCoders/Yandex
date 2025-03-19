ffmpeg:
  pkgrepo.managed:
    - humanname: Ubuntu Multimedia for Trusty
    - name: deb http://ppa.launchpad.net/mc3man/trusty-media/ubuntu trusty main 
    - dist: trusty
    - file: /etc/apt/sources.list.d/multimedia.list
    - keyid: 8E51A6D660CD88D67D65221D90BD7EACED8E640A
    - keyserver: keyserver.ubuntu.com
    - require_in:
      - pkg: ffmpeg
  pkg.installed:
    - refresh: True
