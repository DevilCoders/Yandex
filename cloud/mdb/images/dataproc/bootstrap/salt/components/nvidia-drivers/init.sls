{% set nvidia_repo = 'http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1604/x86_64' %}

{% set kernel_version = salt['grains.get']('kernelrelease') %}
{% set vtype = salt['grains.get']('virtual', 'kvm') %}

{% if salt['ydputils.is_presetup']() %}
nvidia-repository:
    pkgrepo.managed:
        - name: deb {{ nvidia_repo }} /
        - file: /etc/apt/sources.list.d/nvidia.list
        - key_url: {{ nvidia_repo }}/7fa2af80.pub
        - gpgcheck: 1
        - refresh: True

nvidia_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - cuda-drivers
{% if vtype == 'kvm' and kernel_version %}
            - linux-headers-{{ kernel_version }}
{% endif %}
{% endif %}
