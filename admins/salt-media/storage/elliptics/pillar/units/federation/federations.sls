federations:
  1:
    # Fix me
    die_limit: 0
    metadata_db_prefix: ""
    lock_path_prefix: "/mastermind/locks/"
    mm_hosts: {{ salt['conductor']['groups2hosts']('elliptics-cloud') }}
    collector:
      hosts: {{ salt['conductor']['groups2hosts']('elliptics-collector-stable') }}
      hosts_with_prestable: {{ salt['conductor']['groups2hosts']('elliptics-collector') }}
      ignore_empty_snapshot: false
    remotes:
      - 's01man.storage.yandex.net'
      - 's44man.storage.yandex.net'
      - 's88man.storage.yandex.net'
      - 's01vla.storage.yandex.net'
      - 's44vla.storage.yandex.net'
      - 's88vla.storage.yandex.net'
      - 's01myt.storage.yandex.net'
      - 's44myt.storage.yandex.net'
      - 's88myt.storage.yandex.net'
      - 's01iva.storage.yandex.net'
      - 's44iva.storage.yandex.net'
      - 's88iva.storage.yandex.net'
      - 's01sas.storage.yandex.net'
      - 's44sas.storage.yandex.net'
      - 's88sas.storage.yandex.net'
    federation_conductor_groups:
      default: elliptics-storage
      ycloud: elliptics_ycloud-storage
    allowed_to_set_host_federations: ["default", "ycloud", "mandatory_man"]
    karl:
      control_host: 'karl-control.mds.yandex.net:17896'
      meta_hosts: {{ salt['conductor']['groups2hosts']('db_mdba5luqptfb3f8g9nnm') }}
    zk:
      hosts:
        - 'zk01vla.mds.yandex.net:2181'
        - 'zk01myt.mds.yandex.net:2181'
        - 'zk01sas.mds.yandex.net:2181'
    yarl:
      upstreams:
        - 'ubvb3h3hjyqk3u3o.sas.yp-c.yandex.net'
        - 'sp3ovyppcbtgyku3.vla.yp-c.yandex.net'
        - 'qwdmihmayz3c5agd.iva.yp-c.yandex.net'
    state_builder:
      enable_load_cache_update: true
    monolith:
      enable_load_cache_update: false
      # Fix me
      enable_load_cache_update_prestable: true
    scheduler:
      enable_load_cache_update: true
      move_starter:
        distribute_load_phase_enable: true
    mongo:
      mdb_url_template: 'mongodb://{user}:{password}@myt-pepgl6r1z7o4tp41.db.yandex.net:27018,sas-fu34pfc3e9jytnzq.db.yandex.net:27018,vla-qne4ozetgjbtb3dy.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
      all_federations_mdb_url_template: 'mongodb://{user}:{password}@myt-8rqusr1jfqykg5ra.db.yandex.net:27018,sas-iv28le9taohliwkg.db.yandex.net:27018,vla-unjhzq153gu9edxu.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
  2:
    # Fix me
    die_limit: 0
    metadata_db_prefix: "_2"
    lock_path_prefix: "/mastermind/2/locks/"
    mm_hosts: {{ salt['conductor']['groups2hosts']('elliptics-cloud-federation-2') }}
    collector:
      hosts: {{ salt['conductor']['groups2hosts']('elliptics-collector-federation-2') }}
      hosts_with_prestable: {{ salt['conductor']['groups2hosts']('elliptics-collector-federation-2') }}
      ignore_empty_snapshot: true
    remotes: {{ salt['conductor']['groups2hosts']('elliptics-storage-federation-2') }}
    federation_conductor_groups:
      '2': 'elliptics-storage-federation-2'
    allowed_to_set_host_federations: ["2"]
    karl:
      control_host: 'karl-control-2.mds.yandex.net:17896'
      meta_hosts: {{ salt['conductor']['groups2hosts']('db_mdbd4f8c8cuc3dqiofg8') }}
    zk:
      hosts:
        - 'zk01vla.mds.yandex.net:2181'
        - 'zk01myt.mds.yandex.net:2181'
        - 'zk01sas.mds.yandex.net:2181'
    yarl:
      upstreams:
        - 'ubvb3h3hjyqk3u3o.sas.yp-c.yandex.net'
        - 'sp3ovyppcbtgyku3.vla.yp-c.yandex.net'
        - 'qwdmihmayz3c5agd.iva.yp-c.yandex.net'
    state_builder:
      enable_load_cache_update: false
    monolith:
      enable_load_cache_update: false
    scheduler:
      enable_load_cache_update: false
      move_starter:
        distribute_load_phase_enable: false
    mongo:
      mdb_url_template: 'mongodb://{user}:{password}@myt-21fgvced8zjetis6.db.yandex.net:27018,sas-9bhf5c00ugokhzc4.db.yandex.net:27018,vla-3zgoskevr6pjvx39.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
      all_federations_mdb_url_template: 'mongodb://{user}:{password}@myt-8rqusr1jfqykg5ra.db.yandex.net:27018,sas-iv28le9taohliwkg.db.yandex.net:27018,vla-unjhzq153gu9edxu.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
