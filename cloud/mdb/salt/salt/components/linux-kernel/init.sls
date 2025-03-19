# see https://st.yandex-team.ru/MDB-2754
{% set osrelease = salt['grains.get']('osrelease') %}
{% set override_version = salt['pillar.get']('data:linux_kernel:version', None) %}
{% if override_version %}
{%   set kernel_version = override_version %}
{% else %}
{%   set kernel_version = '4.19.143-37' %}
{% endif %}

kernel_dbgsym_purged:
  pkg.purged:
    - pkgs:
      - 'linux-image-{{ kernel_version }}-dbgsym'

kernel_install:
  pkg.installed:
    - refresh: False
    - pkgs:
      - yandex-coroner: 1.1
      - linux-image-server: "{{ kernel_version }}"
      - linux-tools: "{{ kernel_version }}"
      - linux-image-generic: "{{ kernel_version }}"
    - prereq_in:
      - cmd: repositories-ready

{# needed for monitoring in https://st.yandex-team.ru/RUNTIMECLOUD-5718 #}
/etc/salt_kernel_version:
  file.managed:
    - contents: {{ kernel_version }}

{% set newest_kernel=salt['cmd.shell']('ls /boot/vmlinuz-* | /usr/bin/sort -V').split('\n')[-1] %}
{% set newest_kernel_version = '-'.join(newest_kernel.split('-')[1:]) %}

{% if salt['pkg.version_cmp'](kernel_version, newest_kernel_version) >= 0 %}
  {%- set default_content = 'GRUB_DEFAULT=0' %}
{% else %}
  {%- set default_content = 'GRUB_DEFAULT="Advanced options for Ubuntu>Ubuntu, with Linux ' + kernel_version + '"' %}
{% endif %}
{% if salt['grains.get']('virtual', 'physical') == 'physical' %}
{%   set boot_options = 'GRUB_CMDLINE_LINUX="pti=off spectre_v2=off transparent_hugepage=never nvme_core.multipath=0"' %}
{% else %}
{%   set boot_options = 'GRUB_CMDLINE_LINUX="net.ifnames=0 biosdevname=0 console=ttyS0,115200n8 apparmor=1 security=apparmor transparent_hugepage=never"' %}
{% endif %}

set_boot_default:
  file.replace:
    - name: /etc/default/grub
    - pattern: |
        ^GRUB_DEFAULT=.*
    - count: 1
    - repl: '{{ default_content }}\n'
    - append_if_not_found: True
    - require:
      - pkg: kernel_install

set_boot_options:
  file.replace:
    - name: /etc/default/grub
    - pattern: |
        ^GRUB_CMDLINE_LINUX=.*
    - count: 1
    - repl: '{{ boot_options }}\n'
    - append_if_not_found: True
    - require:
      - pkg: kernel_install

update_grub:
  cmd.run:
    - name: '/usr/sbin/update-grub'
    - onchanges:
      - file: set_boot_default
      - file: set_boot_options
