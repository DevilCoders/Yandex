vm.max_map_count:
  sysctl.present:
    - name: "vm.max_map_count"
    - value: "655300"
    - config: "/etc/sysctl.d/elliptics.conf"
