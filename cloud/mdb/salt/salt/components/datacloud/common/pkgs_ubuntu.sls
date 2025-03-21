common-packages:
    pkg.installed:
        - pkgs:
            - bash-completion
            - python-boto
            - python-botocore
            - python-concurrent.futures
            - mdb-ssh-keys: '143-1618a01'
            - curl
            - dstat
            - file
            - bind9-host
            - gdb
            - htop
            - iftop
            - iotop
            - iptables
            - less
            - lsof
            - lsscsi
            - mailutils
            - man-db
            - netcat-openbsd
            - ntpdate
            - postfix
            - psmisc
            - pv
            - rsync
            - retry
        - prereq_in:
            - cmd: repositories-ready
