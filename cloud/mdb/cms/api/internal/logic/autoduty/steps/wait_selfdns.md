# Self dns propagates new ips

If IP has changed CMS should wait while new IPs of containers on DOM0 are propagated to network and other nodes. This step waits for these changes to apply.
Selfdns runs once per 5 minute. DNS changes are propagated in 5 minutes interval.
