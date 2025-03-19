{% set yaenv = grains["yandex-environment"] %}
{% set is_stable = yaenv == "production" %}
{% set is_development = yaenv == "development" %}
{% set is_prestable = yaenv == "prestable" %}
{% set is_production = grains["yandex-environment"] in ["production", "prestable"] %}
{% set php_display_errors = "on" if is_development else "off" %}
{% set php_html_errors = "on" if is_development else "off" %}
{% set php_opcache_validate_timestamps = "on" if is_development else "off"  %}
{% set is_master = true if 'master' in salt.grains.get('conductor:group') else false %}
{% set is_touch = true if 'touch-api' in salt.grains.get('conductor:group') else false %}
{% set is_mobile = true if 'mobile-api' in salt.grains.get('conductor:group') else false %}
{% set is_backend = true if 'backend' in salt.grains.get('conductor:group') else false %}
{% set is_dev = true if 'dev' in salt.grains.get('conductor:group') else false %}
{% set memcache_sessions_host = {'development':'127.0.0.1:11211',
                                'qa':'127.0.0.1:11211'}
                               %}
{# CADMIN-10387 #}
{% set redis_sessions_host = {'testing':'c-mdb0kh65oadth72tgbv3.rw.db.yandex.net:6379',
                              'prestable':'c-mdbbq158r91mgrtrncid.rw.db.yandex.net:6379',
                              'production':'c-mdbi0vpv2nmm2m4he8qs.rw.db.yandex.net:6379',
                              'stress':'c-mdb0kh65oadth72tgbv3.rw.db.yandex.net:6379'}
                              %}

{% set redis_sessions_password = {'testing':salt.yav.get("sec-01fqm2gj892f4kgvj4vcfhyggg")["password"],
                                  'prestable':salt.yav.get("sec-01fwgdyfp7fjy1zve5012yeyga")["password"],
                                  'production':salt.yav.get("sec-01fwgdzpdbsrp5z0nct917h9ek")["password"],
                                  'stress':salt.yav.get("sec-01fqm2gj892f4kgvj4vcfhyggg")["password"]}
                                  %}

{% if is_prestable %}
{% set phpfpm_pool_size = 100 %}
{% elif is_mobile %}
{% set phpfpm_pool_size = 45 %}
{% else %}
{% set phpfpm_pool_size = 30 %}
{% endif %}

{% set fpm_request_terminate_timeout = 90 -%}
{% set ini_max_execution_time = 60 -%}
{% set fpm_slow_request_terminate_timeout = 90 -%}

{% if is_master %}
{% set fpm_request_terminate_timeout = 0 -%}
{% set ini_max_execution_time = 300 -%}
{% endif -%}

{% if is_development %}
{% set ini_max_execution_time = 300 -%}
{% endif -%}

# CADMIN-6143, CADMIN-6144
{% if is_touch %}
{% set fpm_request_terminate_timeout = 15 -%}
{% set ini_max_execution_time = 15 -%}
{% set fpm_slow_request_terminate_timeout = 15 -%}
{% endif -%}

{% if is_mobile %}
{% set fpm_slow_request_terminate_timeout = 60 -%}
{% set fpm_request_terminate_timeout = 15 -%}
{% set ini_max_execution_time = 15 -%}
{% endif -%}

{% set session_save_handler = "memcache" %}
{% set session_save_path_proto = "tcp://" %}
{% set memcached_sess_prefix = "memc.sess.key." %}
{% set memcached_sess_locking = 0 %}
{% set memcached_sess_lock_wait = 15000000 %}
{% set memcached_sess_number_of_replicas = 1 %}
{% set memcached_sess_randomize_replica_read = 'Off' %}
{% set memcached_sess_remove_failed = 'Off' %}
{% set memcached_allow_failover = 'Off' %}

{# CADMIN-5948, CADMIN-6274, CADMIN-10387 #}
{% if yaenv in ["testing", "prestable", "production", "stress"] %}
{% set session_save_path_proto = "tcp://" %}
{% set session_save_handler = "redis" %}
{% set memcached_sess_locking = 0 %}
{% endif %}

{# CADMIN-8686 #}
{% if yaenv in ["testing", "production"] %}
{% set memcached_sess_number_of_replicas = 3 %}
{% set memcached_sess_randomize_replica_read = 'Off' %}
{% set memcached_sess_remove_failed = 'On' %}
{% set memcached_allow_failover = 'On' %}
{% endif %}
{# EO CADMIN-8686 #}

{# CADMIN-7887 #}
include:
  - .sudo

packages:
  cluster:
    - kpmail
    - ubic
    - php-common

php:
  version: "7.3"
  fpm_service_name: "php7.3-fpm"
  fpm_bin: "php-fpm7.3"
  fpm_conf: "/etc/php/7.3/fpm/php-fpm.conf"
  ini_conf: "/etc/php/7.3/fpm/php.ini"
  fpm_ubic_name: "php" # CADMIN-5030: separate definition for ubic by per cluster pillar
  cli: # while rendering for cli, will be merged into pillar:php:ini
    geobase:
      'geobase.autoinit' : false
  fpm:
    global:
      pid: /var/run/php-fpm.pid
      error_log: /var/log/php/fpm-www.error.log
    www:
      listen: /var/run/php73-fpm.sock
      'listen.owner': www-data
      'listen.group': www-data
      'listen.mode': 0600
      pm: ondemand
      'pm.max_children': {{ phpfpm_pool_size }}
      'pm.start_servers': {{ phpfpm_pool_size }}
      'pm.min_spare_servers': {{ phpfpm_pool_size }}
      'pm.max_spare_servers': {{ phpfpm_pool_size }}
      'pm.max_requests': 1000
      'pm.status_path': '/phpfpm-status'
      'access.log': /var/log/php/fpm-www.access.log
      'access.format': '"%t %{mili}d %{kilo}M %C%% %m %s %r%Q%q"'
      'catch_workers_output': yes
      'security.limit_extensions': '.php .phtml'
      'request_terminate_timeout': {{ fpm_request_terminate_timeout }}
      {% if is_backend or is_master or is_dev %}
      'include': '/etc/php/7.3/fpm/secrets.conf'
      {% endif %}
    www_slow:
      listen: /var/run/php73-fpm-slow.sock
      'listen.owner': www-data
      'listen.group': www-data
      'listen.mode': 0600
      pm: ondemand
      'pm.max_children': {{ phpfpm_pool_size }}
      'pm.start_servers': {{ phpfpm_pool_size }}
      'pm.min_spare_servers': {{ phpfpm_pool_size }}
      'pm.max_spare_servers': {{ phpfpm_pool_size }}
      'pm.max_requests': 1000
      'pm.status_path': '/phpfpm-status'
      'access.log': /var/log/php/fpm-www-slow.access.log
      'access.format': '"%t %{mili}d %{kilo}M %C%% %m %s %r%Q%q"'
      'catch_workers_output': yes
      'security.limit_extensions': '.php .phtml'
      'request_terminate_timeout': {{ fpm_slow_request_terminate_timeout }}
      {% if is_backend or is_master or is_dev %}
      'include': '/etc/php/7.3/fpm/secrets.conf'
      {% endif %}
  ini:
    PHP:
      expose_php: off
      output_buffering: 4096
      disable_functions: pcntl_alarm,pcntl_fork,pcntl_waitpid,pcntl_wait,pcntl_wifexited,pcntl_wifstopped,pcntl_wifsignaled,pcntl_wexitstatus,pcntl_wtermsig,pcntl_wstopsig,pcntl_signal,pcntl_signal_dispatch,pcntl_get_last_error,pcntl_strerror,pcntl_sigprocmask,pcntl_sigwaitinfo,pcntl_sigtimedwait,pcntl_exec,pcntl_getpriority,pcntl_setpriority
      memory_limit: {{ "16000M" if is_master else "900M" }}
      max_input_time: {{ 3600 if is_master else 180 }}
      max_input_vars: {{ 100000 if is_master else 1000 }}
      max_execution_time: {{ ini_max_execution_time }}
{% if salt["grains.get"]("conductor:group") in ['kp-dev','kp-test-master'] %}
      upload_max_filesize: 700M
{% endif %}
{% if salt["grains.get"]("conductor:group") in ['kp-master'] %}
      upload_max_filesize: 200M
{% endif %}
      error_reporting: E_ALL & ~E_DEPRECATED & ~E_NOTICE & ~E_STRICT
      error_log: syslog
      display_errors: {{ php_display_errors }}
      log_errors: on
      log_errors_max_len: 4096
      html_errors: {{ php_html_errors }}
      variables_order: GPCS
      request_order: GP
      register_argc_argv: off
      post_max_size: 200M
      default_charset: windows-1251
      enable_dl: off
      'url_rewriter.tags': '"a=href,area=href,frame=src,input=src,form=fakeentry"'
    mbstring:
      'mbstring.language': Russian
      'mbstring.internal_encoding': windows-1251
      'mbstring.http_input': windows-1251
      'mbstring.http_output': windows-1251
      'mbstring.detect_order': windows-1251
    geobase:
      'geobase.autoinit' : true
    Date:
      'date.timezone': 'Europe/Moscow'
    Mail:
      sendmail_from: 'robot@kinopoisk.ru'
      sendmail_path: /usr/bin/kpmail
      mail.add_x_header: On
    Mysql:
      'mysql.allow_persistent': off
      'mysql.default_socket': ''
    Session:
      'session.save_handler': {{ session_save_handler }}
{% if yaenv in ["testing", "production", "prestable", "stress"] %}
      'session.save_path': '"{{ session_save_path_proto }}{{ redis_sessions_host[yaenv] }}?auth={{ redis_sessions_password[yaenv] }}"'
{% endif %}
{% if yaenv in ["dev", "qa"]  %}
      'session.save_path': {{session_save_path_proto}}{{memcache_sessions_host[yaenv]}}
{% endif %}
      'session.cookie_domain': '.kinopoisk.ru'
      'session.cookie_httponly': on
      'session.cookie_secure': on
      'session.gc_divisor': 1000
      # 'session.gc_maxlifetime': 86400
      'session.bug_compat_42': off
      'session.bug_compat_warn': off
      'session.entropy_length': 512
      'session.entropy_file': /dev/urandom
      'session.hash_bits_per_character': 5
# CADMIN-7226
{% if is_mobile %}
      'session.cache_limiter': ''
{% endif %}
    memcached:
      'memcached.sess_locking': {{ memcached_sess_locking }}
      'memcached.sess_lock_wait' : {{ memcached_sess_lock_wait }}
      'memcached.sess_prefix': {{ memcached_sess_prefix }}
      'memcached.sess_number_of_replicas': {{ memcached_sess_number_of_replicas }}
      'memcached.sess_randomize_replica_read': {{ memcached_sess_randomize_replica_read }}
      'memcached.sess_remove_failed': {{ memcached_sess_remove_failed }}
      'memcached.allow_failover': {{ memcached_allow_failover }}

    opcache:
      'opcache.enable': yes
      'opcache.enable_cli': off
      'opcache.memory_consumption': 2048
      'opcache.preferred_memory_model': mmap
      'opcache.interned_strings_buffer': 32
      'opcache.max_accelerated_files': 50000
      'opcache.validate_timestamps': {{ php_opcache_validate_timestamps }}
      'opcache.enable_file_override': on
      'opcache.fast_shutdown': on
      'opcache.save_comments': 1
      'opcache.blacklist_filename': /etc/php/opcache-blacklist.txt
{% if yaenv in ["development","testing","prestable"] %} # or grains['conductor']['group'] == 'kp-load-mobile-api' %}
    xdebug:
      'xdebug.remote_enable': on
{% endif %}
  {% if is_touch or is_dev %}
  touch-api-secrets: {{salt.yav.get('sec-01d7y57znqc22cqc3e586egbza[secrets.data.yaml]' if is_production else 'sec-01d7y563bf58397pvtw4f538ws[secrets.data.yaml]')|json}}
  {% endif %}
  {% if is_mobile or is_dev %}
  mobile-api-secrets: {{salt.yav.get('sec-01d7y58qe4vedfqcpddr4y3vg1[secrets.data.yaml]' if is_production else 'sec-01d7y593j0ff5rvhe4e36jyv18[secrets.data.yaml]')|json}}
  {% endif %}
  {% if is_master or is_dev or is_backend %}
  secrets: {{salt.yav.get('sec-01cscxjf7a5ef278737w8e8ynm[secrets.data.yaml]' if is_stable else 'sec-01csc89vqazy19h0qntfrpdn6e[secrets.data.yaml]')|json}}
  yp_token: {{salt.yav.get('sec-01d1x5wyd6ppzk8sw931kavcbg[DCTL_YP_TOKEN]')|json}}
  {% endif %}
  tvm_port: 8090
  js_api_key: {{salt.yav.get('sec-01cw9c081x67r89vnce530wbyz[JS-Maps-API-key]')|json}}
  {% if is_production %}
  redis_cache_hosts: 'myt-jlm2nfczz2fva7y9.db.yandex.net:26379,sas-eyyq5lmhlwlh3v2s.db.yandex.net:26379,vla-w907c9dwhyazf7am.db.yandex.net:26379'
  {% elif is_development %}
  redis_cache_hosts: 'sas-mvul8iikodqzcyzd.db.yandex.net:26379'
  {% else %}
  redis_cache_hosts: 'man-31rsd6ghir4jy60v.db.yandex.net:26379,sas-u91o32b7eee2h07b.db.yandex.net:26379,vla-prmqvqx3a2cxcl9e.db.yandex.net:26379'
  {% endif %}
