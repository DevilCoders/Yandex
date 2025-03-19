federations:
  1:
    # Fix me
    die_limit: 0
    metadata_db_prefix: ""
    lock_path_prefix: "/mastermind/locks/"
    mm_hosts: {{ salt['conductor']['groups2hosts']('elliptics-test-cloud') }}
    collector:
      hosts: {{ salt['conductor']['groups2hosts']('elliptics-test-collector') }}
      hosts_with_prestable: {{ salt['conductor']['groups2hosts']('elliptics-test-collector') }}
      ignore_empty_snapshot: false
    remotes:
      - 's01man.mdst.yandex.net'
      - 's01vla.mdst.yandex.net'
      - 's01myt.mdst.yandex.net'
      - 's01iva.mdst.yandex.net'
      - 's01sas.mdst.yandex.net'
    federation_conductor_groups:
      default: elliptics-test-storage
    allowed_to_set_host_federations: ["default"]
    karl:
      control_host: 'karl-control.mdst.yandex.net:17896'
      meta_hosts: {{ salt['conductor']['groups2hosts']('db_mdbss0mejacgith0pe8a') }}
    zk:
      hosts:
        - 'zk01man.mdst.yandex.net:2181'
        - 'zk01myt.mdst.yandex.net:2181'
        - 'zk01sas.mdst.yandex.net:2181'
    yarl:
      upstreams:
        - '5ibdi6degjzggxdn.vla.yp-c.yandex.net'
        - 'rhdwlm2ylwbvkwpd.sas.yp-c.yandex.net'
        - 'x46433vvau2b7z3z.iva.yp-c.yandex.net'
    state_builder:
      enable_load_cache_update: true
    monolith:
      enable_load_cache_update: true
    scheduler:
      enable_load_cache_update: true
      move_starter:
          distribute_load_phase_enable: true
    mongo:
      mdb_url_template: 'mongodb://{user}:{password}@sas-ftqcrdb36gvjmst7.db.yandex.net:27018,vla-sqpt8sjrlyewl5vm.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
      all_federations_mdb_url_template: 'mongodb://{user}:{password}@sas-ftqcrdb36gvjmst7.db.yandex.net:27018,vla-sqpt8sjrlyewl5vm.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
  2:
    die_limit: 0
    metadata_db_prefix: "_2"
    lock_path_prefix: "/mastermind/2/locks/"
    remotes: {{ salt['conductor']['groups2hosts']('elliptics-test-storage-federation-2') }}
    mm_hosts: {{ salt['conductor']['groups2hosts']('elliptics-test-cloud-federation-2') }}
    collector:
      hosts: {{ salt['conductor']['groups2hosts']('elliptics-test-collector-federation-2') }}
      hosts_with_prestable: {{ salt['conductor']['groups2hosts']('elliptics-test-collector-federation-2') }}
      ignore_empty_snapshot: false
    federation_conductor_groups:
      '2': 'elliptics-test-storage-federation-2'
    allowed_to_set_host_federations: ["2"]
    karl:
      control_host: 'karl-control-2.mdst.yandex.net:17896'
      # TODO: Fix
      meta_hosts: {{ salt['conductor']['groups2hosts']('db_mdbban6abaj7p8ema29b') }}
    zk:
      hosts:
        - 'zk01man.mdst.yandex.net:2181'
        - 'zk01myt.mdst.yandex.net:2181'
        - 'zk01sas.mdst.yandex.net:2181'
    yarl:
      upstreams:
        - '5ibdi6degjzggxdn.vla.yp-c.yandex.net'
        - 'rhdwlm2ylwbvkwpd.sas.yp-c.yandex.net'
        - 'x46433vvau2b7z3z.iva.yp-c.yandex.net'
    state_builder:
      enable_load_cache_update: false
    monolith:
      enable_load_cache_update: false
    scheduler:
      enable_load_cache_update: false
      move_starter:
        distribute_load_phase_enable: false
    mongo:
      mdb_url_template: 'mongodb://{user}:{password}@sas-ftqcrdb36gvjmst7.db.yandex.net:27018,vla-sqpt8sjrlyewl5vm.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
      all_federations_mdb_url_template: 'mongodb://{user}:{password}@sas-ftqcrdb36gvjmst7.db.yandex.net:27018,vla-sqpt8sjrlyewl5vm.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
  3:
    die_limit: 0
    metadata_db_prefix: "_3"
    lock_path_prefix: "/mastermind/3/locks/"
    remotes:
      - 'jqhzh6nrqzyfgr25.sas.yp-c.yandex.net'
      - 't6yo2hzsqoeen77z.sas.yp-c.yandex.net'
      - 'hhgbzr62uebcurjn.vla.yp-c.yandex.net'
      - 'y22twi4nwjaytplq.vla.yp-c.yandex.net'
    mm_hosts: {{ salt['conductor']['groups2hosts']('elliptics-test-cloud-federation-3') }}
    collector:
      hosts: {{ salt['conductor']['groups2hosts']('elliptics-test-collector-federation-3') }}
      hosts_with_prestable: {{ salt['conductor']['groups2hosts']('elliptics-test-collector-federation-3') }}
      ignore_empty_snapshot: true
    federation_conductor_groups:
      '3': 'elliptics-test-storage-federation-3'
    allowed_to_set_host_federations: ["3"]
    karl:
      control_host: 'karl-control-3.mdst.yandex.net:17896'
      # TODO: Fix
      meta_hosts: {{ salt['conductor']['groups2hosts']('db_mdbucoa5iian1vo5d02k') }}
    zk:
      hosts:
        - 'zk01man.mdst.yandex.net:2181'
        - 'zk01myt.mdst.yandex.net:2181'
        - 'zk01sas.mdst.yandex.net:2181'
    yarl:
      upstreams:
        - '5ibdi6degjzggxdn.vla.yp-c.yandex.net'
        - 'rhdwlm2ylwbvkwpd.sas.yp-c.yandex.net'
        - 'x46433vvau2b7z3z.iva.yp-c.yandex.net'
    state_builder:
      enable_load_cache_update: false
    monolith:
      enable_load_cache_update: false
    scheduler:
      enable_load_cache_update: false
      move_starter:
        distribute_load_phase_enable: false
    mongo:
      mdb_url_template: 'mongodb://{user}:{password}@sas-ftqcrdb36gvjmst7.db.yandex.net:27018,vla-sqpt8sjrlyewl5vm.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
      all_federations_mdb_url_template: 'mongodb://{user}:{password}@sas-ftqcrdb36gvjmst7.db.yandex.net:27018,vla-sqpt8sjrlyewl5vm.db.yandex.net:27018/?replicaSet=rs01&authSource=mastermind'
