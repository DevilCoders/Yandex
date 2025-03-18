package ru.yandex.ds.pgdriver;


import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

public class UrlsTest {

    @Test
    public void asPostgresUrl_cluster() {
        String result = Urls.asPostgresUrl("jdbc:pgcluster://localhost:123/db?a=b");
        Assertions.assertEquals("jdbc:postgresql://localhost:123/db?a=b", result);
    }

    @Test
    public void asPostgresUrl_postgres() {
        String result = Urls.asPostgresUrl("jdbc:postgresql://localhost:123/db?a=b");
        Assertions.assertEquals("jdbc:postgresql://localhost:123/db?a=b", result);
    }

    @Test
    public void asClusterUrl_cluster() {
        String result = Urls.asClusterUrl("jdbc:pgcluster://localhost:123/db?a=b");
        Assertions.assertEquals("jdbc:pgcluster://localhost:123/db?a=b", result);
    }

    @Test
    public void asClusterUrl_postgres() {
        String result = Urls.asClusterUrl("jdbc:postgresql://localhost:123/db?a=b");
        Assertions.assertEquals("jdbc:pgcluster://localhost:123/db?a=b", result);
    }
}
