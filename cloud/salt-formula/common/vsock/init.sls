{% if grains['virtual'] == "kvm" %}
  {% set vsock_map = {"present":["vmw_vsock_virtio_transport"], "absent": ["vhost_vsock"]} %}
  {% for action, modules in vsock_map.items() %}
kvm_{{ action }}_modules:
  kmod.{{ action }}:
    - mods: {{ modules }}
    - persist: True
    {% if action == "absent" %}
      {% for module in modules%}
blacklist-{{ module }}.conf:
  file.managed:
    - name: /etc/modprobe.d/blacklist-{{ module }}.conf
    - contents: "blacklist {{ module }}"
      {% endfor %}
    {% endif %}
  {% endfor %}
{% endif %}
