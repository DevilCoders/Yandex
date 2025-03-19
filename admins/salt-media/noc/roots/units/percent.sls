{%- macro smooth_update(old_ver, new_ver, percent) -%}
  {%- set hnum =  salt['random.seed'](100, hash=None) -%}
  {%- if hnum <= percent -%}
    {{new_ver}}
  {%- else -%}
    {{old_ver}}
  {%- endif -%}
{%- endmacro -%}

{% set __dc = grains["conductor"]["root_datacenter"] %}
{% set __pillar_data = salt["pillar.get"]("salt-autodeploy") %}

{% set sentinel = __pillar_data["sentinel"] %}
{% set __raw_packages = __pillar_data["packages"] %}
{% set __packages_without_dc = __raw_packages|selectattr('dc', 'undefined')|list %}
{% set __packages_with_dc = __raw_packages|selectattr('dc', 'defined')|selectattr('dc', "equalto", __dc)|list %}
{% set __packages_list = __packages_without_dc + __packages_with_dc %}
{% set packages_map = {} %}
{% for pkg in __packages_list %}
  {% do packages_map.update({pkg.name: {"name": pkg.name, "version": smooth_update(pkg.version.old, pkg.version.new, pkg.percent)}}) %}
{% endfor %}
{% set packages = packages_map.values()|list %}

{% if not packages %}
failure:
  test.fail_without_changes:
    - name: "Pillars salt-autodeploy:packages are not defined or empty for this host"
    - failhard: True
{% endif %}

/var/tmp/salt-autodeploy.json:
  file.serialize:
    - dataset:
        dc: {{__dc}}
        sentinel: {{sentinel}}
        packages: {{packages}}
    - formatter: json
    - mode: 0444

/etc/yandex-pkgver-ignore.d/salt-autodeploy-packages:
  file.managed:
    - makedirs: True
    - contents: |
        {%- for pkg in packages %}
        {{pkg.name}}
        {%- endfor %}
