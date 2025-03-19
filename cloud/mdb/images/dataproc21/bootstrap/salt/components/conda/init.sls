{% set any = 'any' %}

{% import 'components/conda/packages.sls' as packages with context %}
{% set packs = packages.config_site['conda'] %}
conda-environment:
     conda.pkg_present:
        - parallel: True
        - packages:
          {% for pkg, version in packs.items() %}
            '{{ pkg }}': '{{ version }}'
          {% endfor %}
