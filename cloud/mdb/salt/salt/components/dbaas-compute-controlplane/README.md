### MDB controlplane common compute packages and configs
#### Network
Add `data:network:l3_slb:virtual_ipv6` with your balancer's ip to your pillar if you need to use your own balancer. Doing so will add balancer's ip to loopback interface.