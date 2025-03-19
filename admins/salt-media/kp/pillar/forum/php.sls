packages:
  cluster:
    - yandex-conf-repo-php-ondrej
    - php-common
    - php-imagick
    - php-memcache
    - php5.6-bcmath
    - php5.6-bz2
    - php5.6-cli
    - php5.6-common
    - php5.6-curl
    - php5.6-dba
    - php5.6-dev
    - php5.6-fpm
    - php5.6-gd
    - php5.6-imap
    - php5.6-intl
    - php5.6-json
    - php5.6-libpuzzle
    - php5.6-mbstring
    - php5.6-mysql
    - php5.6-opcache
    - php5.6-readline
    - php5.6-soap
    - php5.6-sqlite3
    - php5.6-timezonedb
    - php5.6-translit
    - php5.6-xml
    - php5.6-xmlrpc
    - php5.6-xsl
    - php5.6-zip


php:
  fpm_service_name: "php5.6-fpm"
  fpm_bin: "php-fpm5.6"
  fpm_conf: "/etc/php/5.6/fpm/php-fpm.conf"
  ini_conf: "/etc/php/5.6/fpm/php.ini"
  fpm:
    global:
      pid: /var/run/php-fpm.pid
      error_log: syslog
    www:
      listen: /var/run/php5-fpm.sock
      'listen.owner': www-data
      'listen.group': www-data
      'listen.mode': 0600
      pm: static
      'pm.max_children': 10
      'pm.start_servers': 10
      'pm.min_spare_servers': 10
      'pm.max_spare_servers': 10
      'pm.max_requests': 10000
      'access.log': /var/log/php/fpm-www.access.log
      'access.format': '"%t %{mili}d %{kilo}M %C%% %m %s %r%Q%q"'
      catch_workers_output: yes
      'security.limit_extensions': '.php .phtml'
  ini:
    PHP:
      expose_php: off
      output_buffering: 4096
      disable_functions: pcntl_alarm,pcntl_fork,pcntl_waitpid,pcntl_wait,pcntl_wifexited,pcntl_wifstopped,pcntl_wifsignaled,pcntl_wexitstatus,pcntl_wtermsig,pcntl_wstopsig,pcntl_signal,pcntl_signal_dispatch,pcntl_get_last_error,pcntl_strerror,pcntl_sigprocmask,pcntl_sigwaitinfo,pcntl_sigtimedwait,pcntl_exec,pcntl_getpriority,pcntl_setpriority
      max_input_time: 30
      max_execution_time: 30
      memory_limit: "256M"
      max_input_vars: 1000
      error_reporting: E_ALL & ~E_DEPRECATED & ~E_NOTICE & ~E_STRICT
      error_log: /var/log/php/php.log
      display_errors: off
      log_errors: on
      log_errors_max_len: 4096
      html_errors: off
      variables_order: GPCS
      request_order: GP
      register_argc_argv: off
      post_max_size: 200M
      upload_max_filesize: 200M
      default_charset: windows-1251
      enable_dl: off
      'url_rewriter.tags': '"a=href,area=href,frame=src,input=src,form=fakeentry"'
    mbstring:
      'mbstring.language': Russian
      'mbstring.internal_encoding': windows-1251
      'mbstring.http_input': windows-1251
      'mbstring.http_output': windows-1251
      'mbstring.detect_order': windows-1251
    Date:
      'date.timezone': 'Europe/Moscow'
    Mail:
      sendmail_from: 'info@kinopoisk.ru'
      sendmail_path: /usr/sbin/sendmail -t -i -r info@kinopoisk.ru -f info@kinopoisk.ru
      mail.add_x_header: On
    Mysql:
      'mysql.allow_persistent': off
      'mysql.default_socket': ''
    Session:
      'session.save_handler': memcache
      'session.save_path': tcp://127.0.0.1:11211
      'session.cookie_domain': '.kinopoisk.ru'
      'session.cookie_httponly': on
      'session.cookie_secure': on
      'session.gc_divisor': 1000
      'session.bug_compat_42': off
      'session.bug_compat_warn': off
      'session.entropy_length': 512
      'session.entropy_file': /dev/urandom
      'session.hash_bits_per_character': 5
    opcache:
      'opcache.enable': no
      'opcache.enable_cli': off
      'opcache.memory_consumption': 2048
      'opcache.preferred_memory_model': mmap
      'opcache.interned_strings_buffer': 32
      'opcache.max_accelerated_files': 50000
      'opcache.validate_timestamps': off
      'opcache.enable_file_override': on
      'opcache.fast_shutdown': on
      'opcache.save_comments': 0
