# Macro defines roles for current instance
# Returns list of roles
# Example: [masternode, datanode]
{% macro roles() -%}
{{ salt['ydputils.roles']() }}
{%- endmacro %}

# Macro appended elem if it's not exists in lst
{% macro list_attach(lst, elem) -%}
    {%- if elem in lst -%}
    {%- else -%}
        {%- do lst.append(elem) -%}
    {%- endif -%}
{{ lst }}
{%- endmacro %}

# Macro returns version of package from pillar or 'any'
{% macro version(package) -%}
{%- set version = salt['pillar.get']('data:versions:' + package, 'any') -%}
{{ version }}
{%- endmacro %}

# Macro creates salt states with downloading, or installing packages
{% macro pkg_present(name, packages) -%}
{%- if salt['ydputils.is_presetup']() %}
{{ name }}_packages:
    cmd.run:
        - name: apt-get install -y -d {{ salt['ydputils.pkg_download_list'](packages) }}
{%- else -%}
{%- set pkgs = salt['ydputils.pkg_install_dict'](packages) -%}
{%- if pkgs|length > 0 %}
{{ name }}_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
        {% for package, ver in pkgs.items() %}
            - {{ package }}{% if ver != 'any' %}: {{ ver }}{% endif %}
        {% endfor %}
{% endif -%}
{%- endif %}
{%- endmacro %}
