#jinja2: trim_blocks:True, lstrip_blocks:True


include:
  - .network_interfaces_configuration
  - .controlplane_configuration
  - .gobgp_configuration
  - .announcer_configuration