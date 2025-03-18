package ru.yandex.ds.pgdriver;

import java.util.Collection;
import java.util.EnumSet;
import java.util.Properties;

import javax.annotation.Nonnull;

import org.postgresql.PGProperty;
import org.postgresql.util.URLCoder;

class UrlUtils {

    private static final Collection<PGProperty> SYSTEM_PROPERTIES = EnumSet.of(
            PGProperty.PG_DBNAME,
            PGProperty.PG_HOST,
            PGProperty.PG_PORT
    );

    private static final Collection<PGProperty> HOST_SYSTEM_PROPERTIES = EnumSet.of(
            PGProperty.PG_DBNAME,
            PGProperty.PG_HOST,
            PGProperty.PG_PORT,
            PGProperty.TARGET_SERVER_TYPE
    );

    private static final Collection<PGClusterProperty> ALL_CLUSTER_PROPERTIES =
            EnumSet.allOf(PGClusterProperty.class);
    private static final Collection<PGClusterProperty> NONE_CLUSTER_PROPERTIES =
            EnumSet.noneOf(PGClusterProperty.class);

    private UrlUtils() {
    }

    public static String getClusterUrl(Properties properties) {
        StringBuilder url = new StringBuilder(100);
        url.append(Urls.JDBC_CLUSTER_PREFIX).append("//");

        String[] serverNames = PGProperty.PG_HOST.get(properties).split(",");
        String[] portNumbers = PGProperty.PG_PORT.get(properties).split(",");

        for (int i = 0; i < serverNames.length; ++i) {
            if (0 < i) {
                url.append(",");
            }
            url.append(serverNames[i]).append(":").append(portNumbers[i]);
        }

        appendProperties(url, properties, SYSTEM_PROPERTIES, NONE_CLUSTER_PROPERTIES);

        return url.toString();
    }

    /**
     * Возвращает URL-подключения к конкретному инстенсу БД.
     *
     * @param serverName DNS-имя или ip-адрес инстанса БД
     * @param portNumber номер порта
     * @param properties список параметров подключения к БД
     */
    public static String getServerUrl(String serverName, String portNumber, Properties properties) {
        StringBuilder url = new StringBuilder(100);
        url.append(Urls.JDBC_POSTGRESQL_PREFIX).append("//");
        url.append(serverName);
        url.append(':').append(portNumber);

        appendProperties(url, properties, HOST_SYSTEM_PROPERTIES, ALL_CLUSTER_PROPERTIES);

        return url.toString();
    }

    protected static void appendProperties(
            @Nonnull StringBuilder url,
            @Nonnull Properties properties,
            Collection<PGProperty> skipPgProperties,
            Collection<PGClusterProperty> skipClusterProperties
    ) {
        url.append('/');

        String databaseName = PGProperty.PG_DBNAME.get(properties);
        if (null != databaseName) {
            url.append(URLCoder.encode(databaseName));
        }

        boolean firstProperty = true;
        for (PGProperty property : PGProperty.values()) {
            if (skipPgProperties.contains(property)) {
                continue;
            }
            String propertyValue = property.get(properties);
            if (null != propertyValue && !propertyValue.equals(property.getDefaultValue())) {
                if (firstProperty) {
                    firstProperty = false;
                    url.append('?');
                } else {
                    url.append('&');
                }
                url.append(property.getName());
                url.append("=");
                url.append(URLCoder.encode(propertyValue));
            }
        }

        for (PGClusterProperty property : PGClusterProperty.values()) {
            if (skipClusterProperties.contains(property)) {
                continue;
            }
            String propertyValue = property.get(properties);
            if (null != propertyValue && !propertyValue.equals(property.getDefaultValue())) {
                if (firstProperty) {
                    firstProperty = false;
                    url.append('?');
                } else {
                    url.append('&');
                }
                url.append(property.getName());
                url.append("=");
                url.append(URLCoder.encode(propertyValue));
            }
        }
    }
}
