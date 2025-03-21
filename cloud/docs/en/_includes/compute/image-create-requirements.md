Boot disk images must meet the following requirements:

{% if product == "yandex-cloud" %}
* The `virtio-net` and `virtio-blk` drivers are installed. If you're going to attach [file storage](../../compute/concepts/filesystem.md) to a VM, you also need to install the `virtiofs` driver. See the [instructions](../../compute/operations/image-create/custom-image.md#virtio).
{% endif %}
{% if product == "cloud-il" %}
* The `virtio-net`, `virtio-blk` and `virtiofs`. drivers are installed. See the [instructions](../../compute/operations/image-create/custom-image.md#virtio).
{% endif %}
* The `ttyS0` terminal (COM1 port) is set up as a [serial console](../../compute/operations/serial-console/index.md). See the [instructions](../../compute/operations/image-create/custom-image.md#serial-console).
* The network interface runs correctly when starting a VM and obtains the IP address via DHCP.
* The `cloud-init` package is installed and configured to work with our [metadata service](../../compute/operations/vm-info/get-info.md#inside-instance). To install the package for CentOS, run the `sudo yum install cloud-init` command. To install it for Debian or Ubuntu, run `sudo apt update && sudo apt install -y cloud-init`.
* In the system firewall settings, the minimum required set of ports for running your applications and a port for SSH access (by default, this is port 22 TCP) are open.
* The SSH server starts automatically at VM startup.
* Services running your application are resistant to VM reboots.
* GPT disk partitioning is used.
* The disk is mounted by its UUID rather than by name.

