{%- from 'templates/zookeeper/settings.sls' import zk with context -%}

zookeeper_data_in_memory:
  mount.mounted:
    - name: {{zk.data_dir}}
    - device: tmpfs
    - fstype: tmpfs
    - dump: 0
    - pass_num: 0
    - persist: True
    - mkmnt: True
