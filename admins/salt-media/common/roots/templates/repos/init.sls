{% if grains['os_family'] == "Debian" %}
### XXX ONLY FOR DEBIAN
  {% from slspath + "/map.jinja" import repos_common with context %}
  {% from slspath + "/map.jinja" import repos_project with context %}
  {% from slspath + "/map.jinja" import repos with context %}
  {# set repos_map = {"custom": repos, "common": repos_common, "project": repos_project} #}
  {# щит hapens #}
  {%- set repos_map = {} %}
  {%- if repos %} {% do repos_map.update({"custom": repos})%} {% endif %}
  {%- if repos_common %} {% do repos_map.update({"common": repos_common})%} {% endif %}
  {%- if repos_project %} {% do repos_map.update({"project": repos_project})%} {% endif %}
  {# щит happened #}

# XXX Packages
{% for key, val in repos_map.items() if val.packages %}
template-repos-packages {{key}}:
  pkg.installed:
    - pkgs: {{val.packages|json}}
{% endfor %}

# XXX Sources
{% for key, val in repos_map.items() if val.sources %}
  {% for source in val.sources %}
    {% for name, state_content in source.items() %}
template-repos-repos {{ key }} {{name}}:
  pkgrepo.managed:
    {{ state_content|yaml(False)|indent(4) }}
    {% endfor %}
  {% endfor %}
{% endfor %}

{% set pkgs_watch = repos_map.values()|selectattr('packages')|list %}
{% set repos_watch = repos_map.values()|selectattr('sources')|list %}
{% if pkgs_watch or repos_watch %}
# {{pkgs_watch}}
# {{repos_watch}}
repos_update:
  cmd:
    - wait
    - name: apt-get -qq update
    - watch: {# что то в repos_map есть #}
      {% if pkgs_watch %}
      - pkg: template-repos-packages*
      {% endif %}
      {% if repos_watch %}
      - pkgrepo: template-repos-repos*
      {% endif %}
{% endif %}

### XXX ONLY FOR DEBIAN
{% endif %}

