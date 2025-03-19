master:
    private_key: {{ salt.yav.get('sec-01fzzp0qpk4wfewa5t6v950jsp[private_key]') | json }}
salt-arc:
    config-path: /etc/yandex/salt/arc2salt.yaml
    user: root
    group: root
    log_path: /var/log/salt/salt-arc.log
    arc-token: {{ salt.yav.get('sec-01cw6ce3rd7mahtbb89ygxmtf9[arc-token]') | json }}
    monrun_lag_threshold: 180
    monrun_state_file: /var/tmp/salt-arc-monrun.state
    salt_arc_config:
        #arc-token-path: /root/.arc/token
        arc-path: /mnt/arc
        file_roots:
        - '{_ENV_CHECKOUT_PATH_}/strm/salt-configs/roots'
        pillar_roots:
        - '{_ENV_CHECKOUT_PATH_}/strm/salt-configs/pillar'
        - /srv/salt/pillar_static_roots/stable/pillar
        pillar_roots_config_path: /srv/salt/dynamic_pillar_config.yaml
        file_roots_config_path: /srv/salt/dynamic_roots_config.yaml
        env_sources:
          - type: arc_reviews
            options:
                base_checkouts_path: /srv/salt/salt-arc-checkouts
                paths:
                - strm/salt-configs
          - type: arc_static_branches
            options:
                base_checkouts_path: /srv/salt/salt-arc-checkouts
                branches:
                    - trunk
                paths:
                    - strm/salt-configs
          - type: sandbox_releases
            options:
                resource_type: STRM_SALT_STATES
                base_checkouts_path: /srv/salt/salt-arc-checkouts
                releases:
                    testing:
                    - sandbox-testing
                    - testing
                    prestable:
                    - prestable
                    stable:
                    - stable
