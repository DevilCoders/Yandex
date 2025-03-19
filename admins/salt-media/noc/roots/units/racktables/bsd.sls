
/usr/local/etc/newsyslog.conf.d/php-fpm:
  file.managed:
    - user: root
    - mode: 640
    - contents: |
        # logfilename                 [owner:group]    mode count size   when   flags [/pid_file]           [sig_num]
        /var/log/php-fpm-rt-error.log                  644  30    *      @T00   XC    /var/run/php-fpm.pid  SIGUSR1

/usr/local/bin/mysql-reopen-logs:
  file.managed:
    - user: root
    - mode: 755
    - contents: |
        #!/bin/sh
        /usr/local/bin/mysqladmin flush-logs

/usr/local/etc/newsyslog.conf.d/mysql-server:
  file.managed:
    - user: root
    - mode: 640
    - contents: |
        # logfilename                      [owner:group] mode count size   when   flags [/pid_file] [sig_num]
        /base/mysql/noc-myt-slow.log       mysql:mysql   640  14    *      $W0D0  XR    /usr/local/bin/mysql-reopen-logs
        /base/mysql/noc-myt.yandex.net.err mysql:mysql   640  14    *      $W0D0  XR    /usr/local/bin/mysql-reopen-logs

