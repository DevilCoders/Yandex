When a VM is stopped, its public IP address is released, and the next time the VM is started, it gets a new public IP address. When the VM is restarted, its public IP address is saved.

You can make a public IP address static. For more information, see [{#T}](../compute/operations/vm-control/vm-set-static-ip.md).

{% if product == "yandex-cloud" %}

For more information about IP address pricing, see the section [{#T}](../vpc/pricing.md#prices-public-ip) in the {{ vpc-name }} service documentation.

{% endif %}
