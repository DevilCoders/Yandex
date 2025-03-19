package yandex.cloud.codegen;

import java.util.List;

import org.junit.Assert;
import org.junit.Test;
import yandex.cloud.codegen.permission.Iam;

public class CodegenTest {
    @Test
    public void testServices() {
        Assert.assertEquals("resource-manager", Service.RESOURCE_MANAGER.getValue());
        Assert.assertEquals(List.of("cloudai"), Service.AI.getAliases());
    }

    @Test
    public void testResources() {
        Assert.assertEquals("resource-manager.cloud", ResourceTypes.RESOURCE_MANAGER.CLOUD.getValue());
        Assert.assertEquals("resource-manager.cloud", ResourceType.RESOURCE_MANAGER_CLOUD.getValue());
        Assert.assertEquals(Service.RESOURCE_MANAGER, ResourceType.RESOURCE_MANAGER_CLOUD.getService());
    }

    @Test
    public void testQuotas() {
        Assert.assertEquals("compute.disks.count", Quotas.COMPUTE.DISKS_COUNT.getValue());
        Assert.assertEquals("compute.disks.count", Quota.COMPUTE_DISKS_COUNT.getValue());
        Assert.assertEquals(Service.COMPUTE, Quota.COMPUTE_DISKS_COUNT.getService());
    }

    @Test
    public void testPermissions() {
        Assert.assertEquals("iam.serviceAccounts.use", Iam.IAM_SERVICE_ACCOUNTS_USE);
    }

    @Test
    public void testRestrictionKinds() {
        Assert.assertEquals("blockPermissions", RestrictionKind.BLOCK_PERMISSIONS.getValue());
    }

    @Test
    public void testRestrictionTypes() {
        Assert.assertEquals("blockPermissions.freeTier", RestrictionType.BLOCK_PERMISSIONS_FREE_TIER.getValue());
        Assert.assertEquals(RestrictionKind.BLOCK_PERMISSIONS, RestrictionType.BLOCK_PERMISSIONS_FREE_TIER.getKind());
    }
}
