Ansible configs to generate tunnels for Leroy-Merlin
====================================================

## How to add new cluster
1. Add ansible inventory file (see examples in [here](./cl1), [here](./cl2) and [here](./production))
Pay attention to ```hosts_type``` variable ```(regular | tun)```, which can be used to switch between regular and tunneled ips in /etc/hosts
2. Add tunnel ips dict for cluster:
```yaml
---
tunnel_ips:
    {hostname}: {tunnel_ip}
```

## How to deploy
```
ansible-playbook -i {cluster_name} deploy.yml # Deploy tunnels
ansible-playbook -i {cluster_name} hosts.yml # Deploy /etc/hosts
```
