# Metadata on other nodes of cluster and metadata on fqdns

After IP of nodes has changed other node should learn and apply new IP. Otherwise cluster could break.

If any node is UNREACHABLE in juggler, CMS will wait for duty to make all nodes reachable via SSH. This is needed because database clusters are not good at waking up with changed IPs. So CMS forces this strict condition.

After metadata is brought to other nodes, it is OK to post_restart nodes on this dom0 and bring metadata to them with "metadata on fqdns" step.
