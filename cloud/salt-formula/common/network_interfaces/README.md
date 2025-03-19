Everyone who wants add new network interfaces
to svms should include this state.
Here is implemented workflow where we:
- 1) setup interface config in safe location
- 2) stop networking service
- 3) Cleanup `/etc/network/interfaces.d` directory
- 4) copy interface configs to `/etc/network/interfaces.d`
- 5) start networking service again.
And you need require `network_config_init` state to prepare
your own config in safe location
Also you need to require `interfaces_d_clean` state
and then copy your config to /etc/network/interfaces.d.
Also you need extend `networking_start` and `networking_stop`
with your files added for your interfaces

Suggested `/etc/network/interfaces.d` numbering scheme is as follows:
- `1x`: basic configuration, IPVS
- `2x`: additional services (IPv6 for delegated NS, for instance)
