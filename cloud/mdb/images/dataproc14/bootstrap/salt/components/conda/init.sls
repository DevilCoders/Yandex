{% if salt['ydputils.is_presetup']() %}
{% set any = 'any' %}

conda-base-environment:
     conda.pkg_present:
        - packages:
            python: 3.7
            pyarrow: 0.11.1
            ipykernel: 5.1.1
            tensorflow: 1.14.0
            'conda-forge::catboost': 0.15.2-0
            'conda-forge::lightgbm': 2.2.3
            'conda-forge::xgboost': 0.82
            scikit-learn: 0.21.2
            pandas: 0.24.2
            ipython: 7.6.1
            matplotlib: 3.1.0
            py: 1.8.0
            py4j: 0.10.8.1
{% endif %}
