package ru.yandex.ds.pgdriver;

public class Urls {

    public static final String JDBC_CLUSTER_PREFIX = "jdbc:pgcluster:";
    public static final String JDBC_POSTGRESQL_PREFIX = "jdbc:postgresql:";

    private Urls() {
    }

    public static String asPostgresUrl(String url) {
        if (url.startsWith(JDBC_CLUSTER_PREFIX)) {
            return JDBC_POSTGRESQL_PREFIX + url.substring(JDBC_CLUSTER_PREFIX.length());
        }
        if (url.startsWith(JDBC_POSTGRESQL_PREFIX)) {
            return url;
        }
        throw new AssertionError("Wrong URL: " + url);
    }

    public static String asClusterUrl(String url) {
        if (url.startsWith(JDBC_POSTGRESQL_PREFIX)) {
            return JDBC_CLUSTER_PREFIX + url.substring(JDBC_POSTGRESQL_PREFIX.length());
        }
        if (url.startsWith(JDBC_CLUSTER_PREFIX)) {
            return url;
        }
        throw new AssertionError("Wrong URL: " + url);
    }
}
