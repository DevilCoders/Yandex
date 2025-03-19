nodejs_ppa:
  pkgrepo.managed:
    - humanname: NodeSource Node.js Repository
    - name: deb {{ salt['pillar.get']('nodejs:ppa:repository_url', 'https://deb.nodesource.com/node_12.x') }} {{ grains['oscodename'] }} main
    - file: /etc/apt/sources.list.d/nodesource.list
    - keyid: "68576280"
    - key_url: https://deb.nodesource.com/gpgkey/nodesource.gpg.key
    - keyserver: keyserver.ubuntu.com
    - require_in:
      - pkg: nodejs

nodejs:
  pkg.installed:
    - reload_modules: true
    {%- if salt['pillar.get']('nodejs:version', '') %}
    - version: {{ salt['pillar.get']('nodejs:version', '') }}
    {%- endif %}