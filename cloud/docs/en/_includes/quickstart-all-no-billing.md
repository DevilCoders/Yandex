* Infrastructure and network:
  * [Create a Linux VM instance](../compute/quickstart/quick-create-linux.md).
  * [Create a Windows VM instance](../compute/quickstart/quick-create-windows.md).
  * [Creating an instance group](../compute/quickstart/ig.md).
  * [Add files to {{ objstorage-name }}](../storage/quickstart.md).
  * [Create a cloud network in {{ vpc-name }}](../vpc/quickstart.md).
  * [Set up a network load balancer in {{ network-load-balancer-name }}](../network-load-balancer/quickstart.md).
  * [Configure an L7 load balancer in {{ alb-name }}](../application-load-balancer/quickstart.md).
    {% if product == "yandex-cloud" %}* [Set up content distribution over a CDN](../cdn/quickstart.md).{% endif %}
  * [Create DNS zones](../dns/quickstart.md).
  * [Upload a Docker image to a registry in {{ container-registry-name }}](../container-registry/quickstart/index.md).
* Security:
  * [Add users and assign roles in {{ iam-short-name }}](../iam/quickstart.md).
  * [Create a new folder in the cloud and grant access to it in {{ resmgr-name }}](../resource-manager/quickstart.md).
  * [Manage encryption keys in {{ kms-name }}](../kms/quickstart/index.md).
  * [Set up a certificate for a secure connection in {{ certificate-manager-name }}](../certificate-manager/quickstart/index.md).
  * [Store your confidential data securely in {{ lockbox-name }}](../lockbox/quickstart.md).
* Resources and management:
  * [Manage resources in folders and clouds using {{ resmgr-name }}](../resource-manager/quickstart.md).
  * [Set up corporate accounts in {{ org-name }}](../organization/quick-start.md).
  {% if audience != "internal" %}
  * [Set up metrics and monitor your resources using {{ monitoring-name }}](../monitoring/quickstart.md).
  {% endif %}
  {% if audience != "internal" and product == "yandex-cloud" %}
  * [Visualize your data in {{ datalens-name }}](../datalens/quickstart.md).
  {% endif %}
  {% if product == "yandex-cloud" %}
  * [Collect resource logs using {{ cloud-logging-name }}](../logging/quickstart.md).
  {% endif %}
* Container-based development:
  * [Create a {{ k8s }} cluster](../managed-kubernetes/quickstart.md).
  * [Create a registry of your Docker images in {{ container-registry-name }}](../container-registry/quickstart/index.md).
* Serverless computing:
  {% if product == "yandex-cloud" %}* [Run your code as functions in {{ sf-name }}](../functions/quickstart/index.md).{% endif %}
  {% if product == "yandex-cloud" %}* [Use {{ api-gw-name }} to integrate {{ yandex-cloud }} services with other cloud platforms](../api-gateway/quickstart/index.md).{% endif %}
  * [Set up queues to enable messaging between applications with {{ message-queue-name }}](../message-queue/quickstart.md).
  {% if audience != "internal" and product == "yandex-cloud" %}
  * [Manage data streams in {{ yds-name }}](../data-streams/quickstart/index.md).
  {% endif %}
  {% if product == "yandex-cloud" %}
  * [Use {{ iot-name }} as your framework for smart home development](../iot-core/quickstart.md).
  {% endif %}
* Databases and clusters:
  * [{{ CH }}](../managed-clickhouse/quickstart.md).
  {% if product == "yandex-cloud" %}* [{{ MG }}](../managed-mongodb/quickstart.md).{% endif %}
  * [{{ MY }}](../managed-mysql/quickstart.md).
  {% if product == "yandex-cloud" %}* [{{ RD }}](../managed-redis/quickstart.md).{% endif %}
  * [{{ PG }}](../managed-postgresql/quickstart.md).
  {% if product == "yandex-cloud" %}* [{{ ES }}](../managed-elasticsearch/quickstart.md).{% endif %}
  * [{{ KF }}](../managed-kafka/quickstart.md).
  {% if product == "yandex-cloud" %}* [{{ GP }}](../managed-greenplum/quickstart.md).{% endif %}
  {% if audience != "internal" and product == "yandex-cloud" %}* [{{ ydb-name }}](../ydb/quickstart.md#create-db). {% endif %}
  * [Copy and replicate the data between databases using {{ data-transfer-name }}](../data-transfer/quickstart.md).
{% if product == "yandex-cloud" %}
* Machine learning:
  * [Convert text to speech and vice versa using {{ speechkit-name }}](../speechkit/quickstart.md).
  * [Translate text in {{ translate-name }}](../translate/quickstart.md).
  * [Analyze an image using computer vision in {{ vision-name }}](../vision/quickstart.md).
  * [Train and launch your machine learning models in {{ ml-platform-name }}](../datasphere/quickstart.md).
{% endif %}
