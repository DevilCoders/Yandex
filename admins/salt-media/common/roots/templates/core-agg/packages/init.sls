gnome-shell:
  pkg.installed

yandex-jdk8:
  pkg.installed

ubuntu-gnome-desktop:
  pkg.installed:
    - require:
      - pkg: gnome-shell

lightdm:
  pkg.installed:
    - require:
      - pkg: gnome-shell

/etc/X11/default-display-manager:
  file.managed:
    - contents:
      - /usr/sbin/lightdm
    - require:
      - pkg: lightdm

eclipse-mat:
  archive.extracted:
    - name: /home/cores/Documents/eclipse-mat/
    - source: http://ftp-stud.fht-esslingen.de/pub/Mirrors/eclipse/mat/1.5/rcp/MemoryAnalyzer-1.5.0.20150527-linux.gtk.x86_64.zip
    - source_hash: md5=c8797c88bd1cf9e69731d488bed82c0c
    - archive_format: zip
    - user: cores
    - group: cores

/home/cores/Desktop/MemoryAnalyzer:
  file.symlink:
    - target: /home/cores/Documents/eclipse-mat/mat/MemoryAnalyzer
    - user: cores
    - group: cores
    - require:
      - archive: eclipse-mat

/home/cores/Documents/eclipse-mat/mat/MemoryAnalyzer:
  file.managed:
    - mode: 755
    - require:
      - archive: eclipse-mat

/home/cores/Desktop/cores:
  file.directory:
    - user: cores
    - group: cores
    - mode: 775
    - makedirs: True

/etc/rsyncd.conf:
  file.managed:
    - source: salt://{{ slspath }}/rsyncd.conf
    - user: root
    - group: root
    - mode: 644

/etc/default/rsync:
  file.managed:
    - source: salt://{{ slspath }}/defaults_rsync
    - user: root
    - group: root
    - mode: 644
