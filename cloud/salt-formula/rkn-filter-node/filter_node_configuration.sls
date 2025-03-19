install_python3_pip_package:
    yc_pkg.installed:
      - pkgs:
          - python3-pip

install_filter_node_package:
    yc_pkg.installed:
      - pkgs:
          - yc-rkn-filter-node