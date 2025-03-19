master: {{ masters }}
ipv6: True
log_level: info
# Failover Multimaster http://docs.saltstack.com/en/latest/topics/tutorials/multimaster_pki.html
verify_master_pubkey_sign: True
master_type: failover
master_shuffle: True
master_alive_interval: 30
auth_tries: 3
retry_dns: 0
auth_timeout: 10

recon_default: 50
recon_max: 1000
recon_randomize: True
acceptance_wait_time: 10
random_reauth_delay: 60

### remove after destroy failover in media
#retry_dns: 0
#verify_master_pubkey_sign: True
#master_type: failover
#master_shuffle: True
