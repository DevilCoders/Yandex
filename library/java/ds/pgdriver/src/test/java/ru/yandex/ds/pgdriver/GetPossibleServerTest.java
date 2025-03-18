package ru.yandex.ds.pgdriver;

import java.util.Arrays;
import java.util.Properties;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.postgresql.PGProperty;

public class GetPossibleServerTest {

    private PGClusterDriver.ClusterStatus getClusterStatus(String targetType, boolean loadBalance) throws Exception {
        Properties properties = new Properties();
        properties.setProperty(PGProperty.PG_HOST.getName(), "host-0,host-1,host-2,host-3");
        properties.setProperty(PGProperty.PG_PORT.getName(), "2,3,5,7");
        properties.setProperty(PGProperty.TARGET_SERVER_TYPE.getName(), targetType);
        properties.setProperty(PGProperty.LOAD_BALANCE_HOSTS.getName(), loadBalance ? "true" : "false");


        PGClusterDriver.ClusterStatus clusterStatus = new PGClusterDriver.ClusterStatus("jdbc:pgcluster://host-1:1," +
                "host-2:2,host-3:3/db", properties);
        clusterStatus.setServerStatuses(new ServerStatus[]{
                new ServerStatus("host-0", "2", false, 0, 0),
                new ServerStatus("host-1", "3", true, 5, 23),
                new ServerStatus("host-2", "5", true, Long.MAX_VALUE, 41),
                new ServerStatus("host-3", "7", true, 15, 61)
        });

        return clusterStatus;
    }

    @Test
    public void getMaster() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("master", false);

        String result = status.getUrl();
        Assertions.assertEquals("jdbc:postgresql://host-2:5/", result, "должны получить мастара, а он единственный");
    }

    @Test
    public void getMaster_loadBalance() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("master", true);

        String result = status.getUrl();
        Assertions.assertEquals("jdbc:postgresql://host-2:5/?loadBalanceHosts=true", result,
                "должны получить мастара, а он единственный");
    }

    @Test
    public void getSlave() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("slave", false);

        String result = status.getUrl();
        Assertions.assertEquals("jdbc:postgresql://host-1:3/", result, "должны получить реплику с наименьшим пингом");
    }

    @Test
    public void getSlave_loadBalance() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("slave", true);

        String result = status.getUrl();
        Assertions.assertTrue(Arrays.asList(
                "jdbc:postgresql://host-1:3/?loadBalanceHosts=true",
                "jdbc:postgresql://host-3:7/?loadBalanceHosts=true"
        ).contains(result), "должны получить реплику");
    }

    @Test
    public void getPreferSlave() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("preferSlave", false);

        String result = status.getUrl();
        Assertions.assertEquals("jdbc:postgresql://host-1:3/", result,
                "должны получить реплику с наименьшим пингом. не должны получать мастер т.к. доступны реплики");
    }

    @Test
    public void getPreferSlave_loadBalance() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("preferSlave", true);

        String result = status.getUrl();
        Assertions.assertTrue(Arrays.asList(
                        "jdbc:postgresql://host-1:3/?loadBalanceHosts=true",
                        "jdbc:postgresql://host-3:7/?loadBalanceHosts=true"
                ).contains(result),
                "должны получить реплику. не должны получать мастер т.к. доступны реплики");
    }

    @Test
    public void getPreferSlave_slaveDie() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("preferSlave", false);
        status.setServerStatuses(new ServerStatus[]{
                new ServerStatus("host-0", "2", false, 0, 0),
                new ServerStatus("host-1", "3", false, 5, 23),
                new ServerStatus("host-2", "5", true, Long.MAX_VALUE, 41),
                new ServerStatus("host-3", "7", false, 15, 61)
        });

        String result = status.getUrl();
        Assertions.assertEquals("jdbc:postgresql://host-2:5/", result,
                "должны получить мастера т.к. реплики не доступны");
    }

    @Test
    public void getAny() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("any", false);

        String result = status.getUrl();
        Assertions.assertEquals("jdbc:postgresql://host-1:3/", result, "должны получить ближайший хост");
    }

    @Test
    public void getAny_loadBalance() throws Exception {
        PGClusterDriver.ClusterStatus status = getClusterStatus("any", true);

        String result = status.getUrl();
        Assertions.assertTrue(Arrays.asList(
                        "jdbc:postgresql://host-1:3/?loadBalanceHosts=true",
                        "jdbc:postgresql://host-2:5/?loadBalanceHosts=true",
                        "jdbc:postgresql://host-3:7/?loadBalanceHosts=true"
                ).contains(result),
                "должны получить любой хост");
    }


}
