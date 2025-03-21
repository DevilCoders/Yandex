To create a node group:
1. In the [management console]({{ link-console-main }}), select the folder where you want to create your {{ k8s }} cluster.
1. In the list of services, select **{{ managed-k8s-name }}**.
1. Select the {{ k8s }} cluster to create a node group for.
1. On the {{ k8s }} cluster page, go to the **Node groups** tab.
1. Click **Create node group**.
1. Enter a name and description for the node group.
1. Specify the **{{ k8s }} version** for the node.
1. Specify the number of nodes in the group.
1. Under **Scaling**, select a type:
   * `Fixed`: The number of nodes in the group remains unchanged. Specify the number of nodes in the group.
   * `Automatic`: The number of nodes in the group can be controlled using [automatic cluster scaling](../../managed-kubernetes/concepts/autoscale.md#ca).

     As a result, the following settings become available:
     * **Minimum number of nodes**.
     * **Maximum number of nodes**.
     * **Initial number of nodes** with which the group will be created.

   {% note warning %}

   You can't change the scaling type after you create a node group.

   {% endnote %}

1. Under **Allow when creating and updating**, specify the maximum number of instances that you can exceed and reduce the size of the group by.
1. Under **Computing resources**:
   * Choose a [platform](../../compute/concepts/vm-platforms.md).
   * Specify the required number of vCPUs, [guaranteed vCPU performance](../../compute/concepts/performance-levels.md), and RAM.
   * (optional) Specify that the VM must be [preemptible](../../compute/concepts/preemptible-vm.md).
1. (optional) Under **Placement**, enter a name for a [placement group](../../compute/concepts/placement-groups.md) for your nodes. This setting cannot be edited after the group is created.
1. Under **Storage**:
   * Specify the **Disk type**:
     * **HDD**: Standard network drive. Network block storage on an HDD.
     * **SSD**: Fast network drive. Network block storage on an SSD.
     * **Non-replicated SSD**: Improved-performance network drive. You can only change the size of this type of disk in 93 GB increments.

       {% include [nrd-no-backup-note](nrd-no-backup-note.md) %}

   * Specify the disk size.
1. Under **Network settings**:
   * In the **Public IP** field, choose a method for assigning an IP address:
     * **Auto**: Assign a random IP address from the {{ yandex-cloud }} IP pool.
     * **No address**: Don't assign a public IP address.
   * Select the [security groups](../../vpc/concepts/security-groups.md).

     {% include [security-groups-alert](security-groups-alert.md) %}

   {% if product == "yandex-cloud" %}

   * Specify how nodes should be distributed across availability zones and networks.
   * (optional) Click **Add location** and specify an additional availability zone and network to create nodes in different zones.

   {% endif %}

1. Under **Access**, specify the information required to access the node:
   * Enter the username in the **Login** field.
   * In the **SSH key** field, paste the contents of the [public key file](../../managed-kubernetes/operations/node-connect-ssh.md#creating-ssh-keys).
1. Under **Maintenance window settings**:
   * In the **Maintenance frequency / Disable** field, choose the maintenance window:
     * **Disabled**: Automatic updates are disabled.
     * **Anytime**: Maintenance is allowed at any time.
     * **Daily**: Maintenance is performed during the interval specified in the **Time (UTC) and duration** field.
     * **On selected days**: Maintenance is performed during the interval specified in the **Schedule by day** field.
1. In the **Advanced** section:
   * To be able to edit the [unsafe kernel parameters](../../managed-kubernetes/concepts/index.md#node-group) on the group's nodes, click **Add variable**. To enter the name of each kernel parameter, create a separate field.
   * To set up [taint policies for nodes](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/), use the **Add policy** button. Enter the key, value, and effect of each taint policy in a separate set of fields.
   * To set up [node labels](../../managed-kubernetes/concepts/index.md#node-labels) for a node group, click **Add label**. Enter the key and value of each label in a separate set of fields.
1. Click **Create node group**.