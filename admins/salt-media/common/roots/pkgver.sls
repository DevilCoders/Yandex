pkgver:
  pkg.installed:
    - order: 0            #this hack is to make sure that we have all pkgs from Conductor installed before setting up host
    - pkgs:
      {% for package in salt['conductor.package']() %}
      - {{ package }}
      {% endfor %}
