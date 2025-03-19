include:
  - .apt

google-compute-engine-oslogin:
  pkg.installed:
  - pkgs:
    - google-compute-engine-oslogin
  - require_in:
    # https://packages.ubuntu.com/focal/all/gce-compute-image-packages/filelist
    # Package gce-compute-image-packages installs /etc/apt/apt.conf.d/99ipv4-only which is needed to be removed
    - file: remove ipv4 only

disable google-oslogin-cache.timer:
  service.disabled:
    - name: google-oslogin-cache.timer
    - require:
      - pkg: google-compute-engine-oslogin

stop google-oslogin-cache.timer:
  service.dead:
    - name: google-oslogin-cache.timer
    - require:
      - pkg: google-compute-engine-oslogin

