{%- if salt["pillar.get"]("salt_status") == "master" %}
  {%- if salt["grains.get"]("init") == "systemd" %}
salt-master-service:
  file.managed:
    - user: root
    - group: root
    - makedirs: True
    - names:
      - /etc/systemd/system/salt-master.service:
        - contents: |
            [Unit]
            Description=The Salt Master Server
            Documentation=man:salt-master(1) file:///usr/share/doc/salt/html/contents.html https://docs.saltstack.com/en/latest/contents.html
            After=network.target

            [Service]
            Type=simple
            LimitNOFILE=100000
            NotifyAccess=all
            ExecStart=/usr/bin/salt-master

            [Install]
            WantedBy=multi-user.target
  {%- else %}
# Upstart
salt-master-service:
  file.managed:
    - user: root
    - group: root
    - makedirs: True
    - names:
      - /etc/init/salt-master.conf:
        - contents: |
           description "Salt Master"

           start on (net-device-up
                     and local-filesystems
                     and runlevel [2345])
           stop on runlevel [!2345]
           limit nofile 100000 100000

           script
             # Read configuration variable file if it is present
             [ -f /etc/default/$UPSTART_JOB ] && . /etc/default/$UPSTART_JOB

             # Activate the virtualenv if defined
             [ -f $SALT_USE_VIRTUALENV/bin/activate ] && . $SALT_USE_VIRTUALENV/bin/activate

             exec salt-master
           end script
  {%- endif %}
{%- endif %}
{%- if salt["grains.get"]("init") == "systemd" %}
salt-minion-service:
  file.managed:
    - user: root
    - group: root
    - makedirs: True
    - names:
      - /etc/systemd/system/salt-minion.service:
        - contents: |
            [Unit]
            Description=The Salt Minion
            Documentation=man:salt-minion(1) file:///usr/share/doc/salt/html/contents.html https://docs.saltstack.com/en/latest/contents.html
            After=network.target salt-master.service

            [Service]
            KillMode=process
            Type=notify
            NotifyAccess=all
            LimitNOFILE=8192
            ExecStart=/usr/bin/salt-minion

            [Install]
            WantedBy=multi-user.target
{%- else %}
# Upstart
salt-minion-service:
  file.managed:
    - user: root
    - group: root
    - makedirs: True
    - names:
      - /etc/init/salt-minion.conf:
        - contents: |
            description "Salt Minion"

            start on (net-device-up
                      and local-filesystems
                      and runlevel [2345])
            stop on runlevel [!2345]

            # The respawn in the minion is known to cause problems
            # because if the main minion process dies it has done
            # so most likely for a good reason. Uncomment these
            # two lines to enable respawn
            #respawn
            #respawn limit 10 5

            script
              # Read configuration variable file if it is present
              [ -f /etc/default/$UPSTART_JOB ] && . /etc/default/$UPSTART_JOB

              # Activate the virtualenv if defined
              [ -f $SALT_USE_VIRTUALENV/bin/activate ] && . $SALT_USE_VIRTUALENV/bin/activate

              exec salt-minion
            end script
{%- endif %}
