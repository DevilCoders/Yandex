#!/usr/bin/env python3
"""This module contains library constants."""

from quota.utils.request import gRPCRequest, RestRequest

IDENTITY_URL_GRPC = {'prod': 'ts.private-api.cloud.yandex.net:4282',
                'preprod': 'ts.private-api.cloud-preprod.yandex.net:4282',
                'gpn': ''}

IDENTITY_URL = { 'prod': 'https://iam.api.cloud.yandex.net',
                'preprod': 'https://identity.private-api.cloud-preprod.yandex.net:14336',
                'gpn': 'https://identity.private-api.ycp.gpn.yandexcloud.net:443'}

IDENTITY_PROD_URL = 'https://iam.api.cloud.yandex.net'
IDENTITY_PREPROD_URL = 'https://identity.private-api.cloud-preprod.yandex.net:14336'
ENVIRONMENTS = ('prod', 'preprod')
CONVERTABLE_VALUES = (
    'network-hdd-total-disk-size', 'network-ssd-total-disk-size', 'memory',
                 'total-snapshot-size', 'mdb.hdd.size', 'mdb.ssd.size', 'mdb.memory.size',
                 'storage.volume.size', 'serverless.memory.size', 'managed-kubernetes.memory.size',
                 'managed-kubernetes.disk.size', 'compute.hddDisks.size', 'compute.instanceMemory.size',
                 'compute.snapshots.size', 'compute.ssdDisks.size', 'compute.ssdNonReplicatedDisks.size',
                 'compute.ssdFilesystems.size', 'compute.hddFilesystems.size', 'ydb.dedicatedComputeMemory.size'
)
BAD_STATES = (
    'blocked', 'trial', 'trial_active', 'first_payment_required',
    'trial_suspended', 'suspended', '0', 0, 'payment_not_confirmed',
    'payment_required', 'trial_expired', 'pending', 'new'
)

# "kms": "key-management-service"
SERVICE_ALIASES = {
    "k8s": "kubernetes",
    "cr": "container-registry",
    "ig": "instance-group",
    "rm": "resource-manager",
    "mdb": "managed-database",
    "s3": "object-storage",
    "vpc": "virtual-private-cloud",
    "iot": "internet-of-things",
    "lb": "load-balancer",
    "alb": "application-load-balancer"
}

SERVICES = {
    "compute": {
        "client": gRPCRequest,
        "object": 'Compute',
        "endpoint": {
            "prod": "compute-api.cloud.yandex.net:9051",
            "preprod": "compute-api.cloud-preprod.yandex.net:9051"
        }
    },
    "compute-old": {
        "client": RestRequest,
        "object": 'ComputeOld',
        "endpoint": {
            "prod": "https://iaas.private-api.cloud.yandex.net",
            "preprod": "https://iaas.private-api.cloud-preprod.yandex.net"
        }
    },
    "managed-database": {
        "client": RestRequest,
        "object": 'DBaaS',
        "endpoint": {
            "prod": "https://mdb.private-api.cloud.yandex.net",
            "preprod": "https://mdb.private-api.cloud-preprod.yandex.net"
        }
    },
    "object-storage": {
        "client": RestRequest,
        "object": 'ObjectStorage',
        "endpoint": {
            "prod": "https://storage-idm.private-api.cloud.yandex.net:1443",
            "preprod": "https://storage-idm.private-api.cloud-preprod.yandex.net:1443"
        }
    },
    "resource-manager": {
        "client": gRPCRequest,
        "object": 'ResourceManager',
        "endpoint": {
            "prod": "rm.private-api.cloud.yandex.net:4284",
            "preprod": "rm.private-api.cloud-preprod.yandex.net:4284"
        }
    },
    "kubernetes": {
        "client": gRPCRequest,
        "object": 'Kubernetes',
        "endpoint": {
            "prod": "mk8s.private-api.ycp.cloud.yandex.net:443",
            "preprod": "mk8s.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "triggers": {
        "client": gRPCRequest,
        "object": 'Triggers',
        "endpoint": {
            "prod": "serverless-triggers.private-api.ycp.cloud.yandex.net:443",
            "preprod": "serverless-triggers.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "functions": {
        "client": gRPCRequest,
        "object": 'Functions',
        "endpoint": {
            "prod": "serverless-functions.private-api.ycp.cloud.yandex.net:443",
            "preprod": "serverless-functions.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "instance-group": {
        "client": gRPCRequest,
        "object": 'InstanceGroup',
        "endpoint": {
            "prod": "instance-group.private-api.ycp.cloud.yandex.net:443",
            "preprod": "instance-group.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "container-registry": {
        "client": gRPCRequest,
        "object": 'ContainerRegistry',
        "endpoint": {
            "prod": "container-registry.private-api.ycp.cloud.yandex.net:443",
            "preprod": "container-registry.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "virtual-private-cloud": {
        "client": gRPCRequest,
        "object": 'VirtualPrivateCloud',
        "endpoint": {
            "prod": "api-adapter.private-api.ycp.cloud.yandex.net:443",
            "preprod": "network-api-internal.private-api.cloud-preprod.yandex.net:9823"
        }
    },
    "billing": {
        "client": RestRequest,
        "object": 'Billing',
        "endpoint": {
            "prod": "https://billing.private-api.cloud.yandex.net:16465",
            "preprod": "https://billing.private-api.cloud-preprod.yandex.net:16465"
        }
    },
    "monitoring": {
        "client": gRPCRequest,
        "object": 'Monitoring',
        "endpoint": {
            "prod": "monitoring.private-api.cloud.yandex.net:443",
            "preprod": "monitoring.private-api.cloud-preprod.yandex.net:443"
        }
    },
    "internet-of-things": {
        "client": gRPCRequest,
        "object": 'InternetOfThings',
        "endpoint": {
            "prod": "iot-devices.private-api.ycp.cloud.yandex.net:443",
            "preprod": "iot-devices.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "load-balancer": {
        "client": gRPCRequest,
        "object": 'LoadBalancer',
        "endpoint": {
            "prod": "network-api.private-api.cloud.yandex.net:9823",
            "preprod": "network-api-internal.private-api.cloud-preprod.yandex.net:9823"
        }
    },
    # nans preprod
    "ydb": {
        "client": gRPCRequest,
        "object": 'YDB',
        "endpoint": {
            "prod": "ydbc.ydb.cloud.yandex.net:2135",
            "preprod": "ydbc.ydb.cloud-preprod.yandex.net:2135"
        }
    },
    "application-load-balancer": {
        "client": gRPCRequest,
        "object": 'ALB',
        "endpoint": {
            "prod": "alb.ycp.cloud.yandex.net:443",
            "preprod": "alb.ycp.cloud-preprod.yandex.net:443"
        }
    },
    "dns": {
        "client": gRPCRequest,
        "object": 'DNS',
        "endpoint": {
            "prod": "dns.private-api.ycp.cloud.yandex.net:443",
            "preprod": "dns.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    }

}

UNIMPLIMENTED_SERVICES = {
    "key-management-service": {
        "client": gRPCRequest,
        "object": 'KeyManagementServices',
        "endpoint": {
            "prod": "kms-cpl.private-api.ycp.cloud.yandex.net:443",
            "preprod": "kms-cpl.private-api.ycp.cloud-preprod.yandex.net:443"
        }
    },
}
