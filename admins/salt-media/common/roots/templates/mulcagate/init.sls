{% from slspath + "/map.jinja" import mulcagate with context %}

/etc/mulcagate/mulcagate.conf:
  file.managed:
    - source: {{ mulcagate.config }}
    - mode: 644
    - user: root
    - group: root
    - template: jinja
    - context:
      client_chunk_size: {{ mulcagate.client_chunk_size }}
      backend_chunk_size: {{ mulcagate.backend_chunk_size }}
      io_threads: {{ mulcagate.io_threads }}
      accept_threads: {{ mulcagate.accept_threads }}
      keep_alive: {{ mulcagate.keep_alive }}
      pool_count: {{ mulcagate.pool_count }}
      max_buffers_size: {{ mulcagate.max_buffers_size }}
      reject_initially_deprived_requests: {{ mulcagate.reject_initially_deprived_requests }}
      mulca_write_enabled: {{ mulcagate.mulca_write_enabled }}
      mulca_delete_enabled_for_mapped_units: {{ mulcagate.mulca_delete_enabled_for_mapped_units }}
      put_pool_size: {{ mulcagate.put.pool_size }}
      put_pool_threads: {{ mulcagate.put.pool_threads }}
      put_streaming_threshold: {{ mulcagate.put.streaming_threshold }}
      get_pool_size: {{ mulcagate.conf_get.pool_size }}
      get_pool_threads: {{ mulcagate.conf_get.pool_threads }}
      del_pool_size: {{ mulcagate.del.pool_size }}
      del_pool_threads: {{ mulcagate.del.pool_threads }}
      redirect_enabled: {{ mulcagate.redirect_enabled }}
      redirect_options: {{ mulcagate.redirect_options }}
      custom_handystat_config: {{ mulcagate.custom_handystat_config }}
      small_data_compression_threshold: {{ mulcagate.small_data_compression_threshold }}
      small_data_compression_namespaces: {{ mulcagate.small_data_compression_namespaces }}
      elliptics_prefer_couple_from_unit_mapping: {{ mulcagate.elliptics_prefer_couple_from_unit_mapping }}
      disable_elliptics: {{ mulcagate.disable_elliptics }}
      {% if grains['yandex-environment'] in ['testing'] %}
      elliptics_remotes: {{ mulcagate.elliptics.remotes_testing }}
      {% else %}
      elliptics_remotes: {{ mulcagate.elliptics.remotes }}
      {% endif %}
      elliptics_auth: {{ mulcagate.elliptics.auth }}
      {% if grains['yandex-environment'] in ['testing'] %}
      elliptics_auth_options: {{ mulcagate.elliptics.auth_client_id_testing }}
      {% else %}
      elliptics_auth_options: {{ mulcagate.elliptics.auth_client_id }}
      {% endif %}
      elliptics_log_level: {{ mulcagate.elliptics.log_level }}
      elliptics_timeout_default: {{ mulcagate.elliptics.timeout.default }}
      elliptics_timeout_read: {{ mulcagate.elliptics.timeout.read }}
      elliptics_timeout_write: {{ mulcagate.elliptics.timeout.write }}
      elliptics_timeout_write_prepare: {{ mulcagate.elliptics.timeout.write_prepare }}
      elliptics_timeout_lookup: {{ mulcagate.elliptics.timeout.lookup }}
      elliptics_timeout_remove: {{ mulcagate.elliptics.timeout.remove }}
      elliptics_io_thread_num: {{ mulcagate.elliptics.io_thread_num }}
      elliptics_nonblocking_io_thread_num: {{ mulcagate.elliptics.nonblocking_io_thread_num }}
      elliptics_net_thread_num: {{ mulcagate.elliptics.net_thread_num }}
      elliptics_mds2079_lower_bound: {{ mulcagate.elliptics.mds2079_lower_bound }}
      elliptics_mastermind_update_period: {{ mulcagate.elliptics.mastermind.update_period }}
      elliptics_mastermind_worker_name: {{ mulcagate.elliptics.mastermind.worker_name }}
      {% if grains['yandex-environment'] in ['testing'] %}
      elliptics_mastermind_remotes: {{ mulcagate.elliptics.mastermind.remotes_testing }}
      {% else %}
      elliptics_mastermind_remotes: {{ mulcagate.elliptics.mastermind.remotes }}
      {% endif %}
      elliptics_mastermind_expire_time: {{ mulcagate.elliptics.mastermind.expire_time }}
      elliptics_mastermind_enqueue_retries: {{ mulcagate.elliptics.mastermind.enqueue_retries }}
      elliptics_libmds_thread_num: {{ mulcagate.elliptics.libmds.thread_num }}
      elliptics_redirect_check_port: {{ mulcagate.elliptics.libmds.redirect_check_port }}
      elliptics_redirect_check_query: {{ mulcagate.elliptics.libmds.redirect_check_query }}
      elliptics_redirect_check_timeout_ms: {{ mulcagate.elliptics.libmds.redirect_check_timeout_ms }}

/usr/local/bin/elliptics-read-file-status.py:
  file.managed:
    - source: salt://templates/mulcagate/files/usr/local/bin/elliptics-read-file-status.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/etc/yandex/loggiver/pattern.d/elliptics-read-file-status.pattern:
  file.managed:
    - source: salt://templates/mulcagate/files/etc/yandex/loggiver/pattern.d/elliptics-read-file-status.pattern
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

libmastermind_cache_freshness:
  monrun.present:
    - command: "/usr/local/bin/libmastermind_cache_freshness.sh -m 3600"
    - execution_interval: 300
    - execution_timeout: 60
    - type: mulcagate

  file.managed:
    - name: /usr/local/bin/libmastermind_cache_freshness.sh
    - source: salt://templates/mulcagate/files/usr/local/bin/libmastermind_cache_freshness.sh
    - user: root
    - group: root
    - mode: 755

include:
  - templates.libmastermind_cache

libmastermind_cache:
  monrun.present:
    - command: "timetail -t mulcagate /var/log/mulcagate/yplatform.log |grep libmastermind | /usr/local/bin/libmastermind_cache.py  --ignore=cached_keys"
    - execution_interval: 300
    - execution_timeout: 60
    - type: mulcagate

mulcagate:
  monrun.present:
    {% if grains['yandex-environment'] in ['testing'] %}
    - command: "/usr/bin/http_check.sh gate/get/{{ mulcagate.ping_stid_testing }} 10010"
    {% else %}
    - command: "/usr/bin/http_check.sh gate/get/{{ mulcagate.ping_stid }} 10010"
    {% endif %}
    - execution_interval: 60
    - type: mulcagate

/etc/libmulca/libmulca.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/libmulca/libmulca.conf
    - user: root
    - group: root
    - makedirs: True

/etc/units.txt:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/units.txt
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

{% if pillar.get('mulcagate', {}).get('custom_handystat_config', false) %}
/etc/mulcagate/handystats.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/mulcagate/handystats.conf
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endif %}
