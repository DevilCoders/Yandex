def test_quotas():
    from yc_constants import quotas
    assert quotas.COMPUTE_DISKS_COUNT == "compute.disks.count"


def test_resources():
    from yc_constants import resources
    assert resources.RESOURCE_MANAGER_CLOUD == "resource-manager.cloud"


def test_permissions():
    from yc_constants import permissions
    assert permissions.IAM_SERVICE_ACCOUNTS_USE == "iam.serviceAccounts.use"
