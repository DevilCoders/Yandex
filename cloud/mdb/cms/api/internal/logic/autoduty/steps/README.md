# Steps of autoduty


Autoduty is super cautious, you can always override it's decision.

Each step explained:
* [Register minion](register_minion.md)
* [Host created in conductor](create_host.md)
* [whip primaries away](ensure_no_primary.md)
* ["ip changed?" and "remember dom0 state"](ipchange.md)
* [replace detail and return?](let_go_part_change.md)
* [locks for dom0 and containers](mlock.md)
* [unregister minion](unregister.md)
* [all clusters are not at Wall-e](wait_until_walle_frees_cluster.md)
* [health of clusters](wait_for_healthy.md)
* [ip changed](ipchange.md)
* [whip primaries away](ensure_no_primary.md)
* [wait selfdns](wait_selfdns.md)
* [is dom0 unreachable?](let_go_unreachable.md)
* [wait containers and dom0 reachable](wait_containers_reachable.md)
* [other legs reachable](other_legs_reachable.md)
* ["metadata on other nodes" and "metadata on fqdns"](metadata.md)
* [check next drills](drills.md)
* steps with [shipments](shipments.md) say what they do:
  * pre_restart
  * post_restart
  * start_containers
  * stop_containers
