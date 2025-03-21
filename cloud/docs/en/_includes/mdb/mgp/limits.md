## Quotas {#quotas}

| Type of limit                                              | Value   |
|------------------------------------------------------------|---------|
| Number of clusters per cloud                               | 16      |
| Total number of processor cores for all DB hosts per cloud | 96      |
| Total virtual memory for all DB hosts per cloud            | 640 GB  |
| Total storage capacity for all clusters per cloud          | 4096 GB |

## Limits {#limits}

{% if state == "public-preview" %}

| Type of limit | Minimum value | Maximum value |
|:-------------------------------------------------------------------------------------------------------------|:-----------------------------------------------|:-------------------------------------------------------------|
| Host class | s2.micro (2 vCPU Intel Cascade Lake, 8 GB RAM) | m3-c80-m640 (80 vCPU Intel Ice Lake, 640 GB RAM) |
| Number of master hosts per cluster | 1 | 2 |
| Number of segment hosts per cluster | 2 | 32 |
| Number of segments per host | 1 | Is equal to the number of vCPUs for the selected class of host segments |
| Amount of data on the host when using HDD network storage | 10 GB | 2048 GB |
| Amount of data on the host when using SSD network storage | 10 GB | 4096 GB |
| Amount of data on the host when using local SSD storage (for the Intel Cascade Lake) | 100 GB | 1500 GB |
| Amount of data on the host when using local SSD storage (for Intel Ice Lake) | {{ local-ssd-v3-step }} | {{ local-ssd-v3-max }} |

{% else %}

| Type of limit | Minimum value | Maximum value |
|:--------------------------------------------------------------------------------------------------------------|:-----------------------------------------------|:-------------------------------------------------------------|
| Host class | s2.micro (2 vCPU Intel Cascade Lake, 8 GB RAM) | m2.9xlarge (80 vCPU Intel Cascade Lake, 640 GB RAM) |
| Number of master hosts per cluster | 1 | 2 |
| Number of segment hosts per cluster | 2 | 32 |
| Number of segments per host | 1 | Is equal to the number of vCPUs for the selected class of host segments |
| Amount of data on the host when using HDD network storage | 10 GB | 2048 GB |
| Amount of data on the host when using SSD network storage | 10 GB | 4096 GB |
| Amount of host data when using non-replicated SSD storage (for segment hosts only) | 93 GB | 8184 GB |
| Amount of data on the host when using local SSD storage (for the Intel Cascade Lake) | 100 GB | 1500 GB |
| Amount of data on the host when using local SSD storage (for Intel Ice Lake) | {{ local-ssd-v3-step }} | {{ local-ssd-v3-max }} |

{% endif %}

