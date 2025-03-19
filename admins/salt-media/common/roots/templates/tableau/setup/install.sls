download-tableau-package:
    cmd.run:
      - name: wget https://downloads.tableau.com/esdalt/{{ salt['pillar.get']('tableau_version') }}/tableau-server-{{ salt['pillar.get']('tableau_version') | replace('.','-') }}_amd64.deb -O /tmp/tableau.deb

install-tableau-package:
    cmd.run:
      - name: gdebi -n /tmp/tableau.deb
