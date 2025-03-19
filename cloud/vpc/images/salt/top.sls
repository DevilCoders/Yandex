base:
  'image:base':
    - match: grain
    - base.apparmor
    - base.apt
    - base.breakglass
    - base.ca
    - base.cloud-init
    - base.curl
    - base.google-compute-engine-oslogin
    - base.network
    - base.nss
    - base.osquery
    - base.packages
    - base.pam
    - base.skm
    - base.sshd
    - base.sudoers
    - base.sysctl
  'image:mr-prober-base':
    - match: grain
    - common.docker
    - common.patch_skm_restart_policy
    - common.disable_release_upgrader
  'image:mr-prober-agent':
    - match: grain
    - common.unified_agent
    - mr-prober.agent.agent
  'image:mr-prober-web-server':
    - match: grain
    - common.unified_agent
    - mr-prober.web-server
  'image:mr-prober-router':
    - match: grain
    - mr-prober.router
  'image:mr-prober-creator':
    - match: grain
    - common.unified_agent
    - mr-prober.creator.creator
    - mr-prober.creator.meeseeks-updater
    - mr-prober.creator.yc
  'image:mr-prober-api':
    - match: grain
    - common.docker_ipv6nat
    - common.unified_agent
    - mr-prober.api.api
  'image:dns-proxy':
    - match: grain
    - dns-proxy.network
    - dns-proxy.apparmor
    - dns-proxy.bind
  'image:monops':
    - match: grain
    - common.docker
    - monops
  'image:vpc-accounting':
    - match: grain
    - common.unified_agent
    - accounting.accounting-service
  'image:vpc-load-test':
    - match: grain
    - common.unified_agent
    - vpc-load-test.jump-host
  'image:e2e-nlb-target':
    - match: grain
    - common.docker
    - e2e.nlb-target.main
