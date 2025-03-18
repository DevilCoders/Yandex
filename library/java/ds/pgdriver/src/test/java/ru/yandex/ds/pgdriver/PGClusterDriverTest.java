package ru.yandex.ds.pgdriver;

import java.sql.DriverPropertyInfo;
import java.util.Arrays;
import java.util.Optional;
import java.util.Properties;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

public class PGClusterDriverTest {

    private PGClusterDriver driver;

    @BeforeEach
    public void setUp() {
        driver = new PGClusterDriver();
    }

    @Test
    public void acceptsURL_pgcluster() throws Exception {
        boolean result = driver.acceptsURL("jdbc:pgcluster://localhost:123/db");
        Assertions.assertTrue(result);
    }

    @Test
    public void acceptsURL_postgresql() throws Exception {
        boolean result = driver.acceptsURL("jdbc:postgresql://localhost:123/db");
        Assertions.assertFalse(result);
    }

    @Test
    public void acceptsURL_pgcluster_withCustomParameter() throws Exception {
        boolean result = driver.acceptsURL("jdbc:pgcluster://localhost:123/db?maxReplicationLag=1000" +
                "&hostRecheckSeconds=5000");
        Assertions.assertTrue(result);
    }

    @Test
    public void getPropertyInfo() throws Exception {
        DriverPropertyInfo[] info = driver.getPropertyInfo("jdbc:pgcluster://localhost:123/db" +
                "?maxReplicationLag=1000&hostRecheckSeconds=5000", new Properties());

        Optional<DriverPropertyInfo> hostRecheckSeconds = Arrays.asList(info).stream()
                .filter(i -> "hostRecheckSeconds".equals(i.name))
                .findFirst();
        Assertions.assertTrue(hostRecheckSeconds.isPresent());
        Assertions.assertEquals("5000", hostRecheckSeconds.get().value);

        Optional<DriverPropertyInfo> maxReplicationLag = Arrays.asList(info).stream()
                .filter(i -> "maxReplicationLag".equals(i.name))
                .findFirst();
        Assertions.assertTrue(maxReplicationLag.isPresent());
        Assertions.assertEquals("1000", maxReplicationLag.get().value);
    }

    @Test
    public void getClusterStatus_maxReplicationLag() throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://localhost:123/db" +
                "?maxReplicationLag=1000", new Properties());

        Assertions.assertNotNull(status);
        Assertions.assertEquals(1000L, status.getMaxReplicationLag());
    }

    @Test
    public void getClusterStatus_hostRecheckSeconds() throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://localhost:123/db" +
                "?hostRecheckSeconds=123", new Properties());

        Assertions.assertNotNull(status);
        Assertions.assertEquals(123000L, status.getHostRecheckPeriod());
    }

    @Test
    public void getClusterStatus_loadBalanceHosts_true() throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://localhost:123/db" +
                "?loadBalanceHosts=true", new Properties());

        Assertions.assertNotNull(status);
        Assertions.assertTrue(status.isLoadBalanceHosts());
    }

    @Test
    public void getClusterStatus_loadBalanceHosts_false() throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://localhost:123/db" +
                "?loadBalanceHosts=false", new Properties());

        Assertions.assertNotNull(status);
        Assertions.assertFalse(status.isLoadBalanceHosts());
    }

    @ParameterizedTest
    @ValueSource(strings = {"any", "primary", "master", "slave", "secondary", "preferSlave", "preferSecondary"})
    public void getClusterStatus_targetServerType(String targetType) throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://localhost:123/db" +
                "?targetServerType=" + targetType, new Properties());

        Assertions.assertNotNull(status);
        Assertions.assertEquals(targetType, status.getTargetServerType());
    }

    @Test
    public void getClusterStatus_getServerNames() throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://host-1:1,host-2:2," +
                "host-3:3/db", new Properties());

        Assertions.assertNotNull(status);
        String[] names = status.getServerNames();
        Assertions.assertEquals("host-1", names[0]);
        Assertions.assertEquals("host-2", names[1]);
        Assertions.assertEquals("host-3", names[2]);
    }

    @Test
    public void getClusterStatus_getPortNumbers() throws Exception {
        PGClusterDriver.ClusterStatus status = driver.getClusterStatus("jdbc:pgcluster://host-1:1,host-2:3," +
                "host-3:7/db", new Properties());

        Assertions.assertNotNull(status);
        String[] ports = status.getPortNumbers();
        Assertions.assertEquals("1", ports[0]);
        Assertions.assertEquals("3", ports[1]);
        Assertions.assertEquals("7", ports[2]);
    }

}
