{% set pkgs = [ "nginx","nginx-full","nginx-common" ] %}
{% set pkgs = salt["conductor.package"](pkgs) or pkgs %}
nginx_pkg:
  pkg.installed:
    - pkgs: {{ pkgs }}
    {% if 'repos' in pillar  %}
    - require:
      - sls: templates.repos
    {% endif %}
