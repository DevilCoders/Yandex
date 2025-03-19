include:
    - components.dbaas-controlplane

porto-resources-grains-pkgs:
    pkg.installed:
        - pkgs:
            - python-portopy: {{ salt.grains.get('porto_version', '4.18.21') }}
            - python3-portopy:  {{ salt.grains.get('porto_version', '4.18.21') }}

