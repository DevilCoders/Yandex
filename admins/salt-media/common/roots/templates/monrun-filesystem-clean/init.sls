
{{ pillar.get('filesystem_clean_check_name', 'filesystem-clean') }}:
  monrun.present:
    - command: "/usr/local/bin/filesystem_clean.sh"
    - execution_interval: 300
    - execution_timeout: 60
    - type: storage

  file.managed:
    - name: /usr/local/bin/filesystem_clean.sh
    - source: salt://templates/monrun-filesystem-clean/filesystem_clean.sh
    - user: root
    - group: root
    - mode: 755
    
monrun_filesystem_clean:
  file.managed:
    - name: /etc/sudoers.d/monrun_filesystem_clean
    - source: salt://templates/monrun-filesystem-clean/monrun_filesystem_clean
    - user: root
    - group: root
    - mode: 440
