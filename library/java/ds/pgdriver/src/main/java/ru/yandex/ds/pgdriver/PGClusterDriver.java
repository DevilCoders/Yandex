package ru.yandex.ds.pgdriver;

import java.sql.Connection;
import java.sql.Driver;
import java.sql.DriverManager;
import java.sql.DriverPropertyInfo;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Stream;

import org.postgresql.PGProperty;
import org.postgresql.ds.common.BaseDataSource;
import org.postgresql.util.PSQLException;


public class PGClusterDriver implements Driver {

    private static final Logger LOGGER = Logger.getLogger(BaseDataSource.class.getName());

    private static final Logger PARENT_LOGGER = Logger.getLogger("org.postgresql");

    /**
     * Период с момента последнего получения соединения для указанного URL в течение которого актуализируется информация
     * о статусче инстансов БД. Если соединение для указанного URL будет получено повторно в течение указанного времени,
     * то оно будет получено быстро т.к. для его получения не потребуется опрашивать статус инстансов.
     */
    private static final long MAX_UNUSED_PERIOD = 5 * 60 * 1000;

    private static final String CHECK_QUERY = "SELECT pg_is_in_recovery(), (EXTRACT(EPOCH FROM (NOW() - " +
            "pg_last_xact_replay_timestamp())) * 1000)::BIGINT";

    private static final Comparator<ServerStatus> NEAREST_HOST_COMPARATOR =
            Comparator.comparingLong(ServerStatus::getPingTime);
    private static final Comparator<ServerStatus> PREFER_SLAVE_COMPARATOR =
            Comparator.comparing(ServerStatus::isMaster)
                    .thenComparingLong(ServerStatus::getPingTime);

    private static final Properties EMPTY_PROPERTIES = new Properties();

    private static final org.postgresql.Driver PG_DRIVER = new org.postgresql.Driver();
    private static final ConcurrentMap<String, ClusterStatus> CONNECTIONS = new ConcurrentHashMap<>();

    private static PGClusterDriver instance;

    static {
        try {
            // moved the registerDriver from the constructor to here
            // because some clients call the driver themselves (I know, as
            // my early jdbc work did - and that was based on other examples).
            // Placing it here, means that the driver is registered once only.
            register();
        } catch (SQLException e) {
            throw new ExceptionInInitializerError(e);
        }
    }

    private static final RefreshThread REFRESH_THREAD;

    static {
        REFRESH_THREAD = new RefreshThread();
        REFRESH_THREAD.start();
    }

    /**
     * Register the driver against {@link DriverManager}. This is done automatically when the class is
     * loaded. Dropping the driver from DriverManager's list is possible using {@link #deregister()}
     * method.
     *
     * @throws IllegalStateException if the driver is already registered
     * @throws SQLException          if registering the driver fails
     */
    public static void register() throws SQLException {
        if (isRegistered()) {
            throw new IllegalStateException(
                    "Driver is already registered. It can only be registered once.");
        }
        PGClusterDriver registeredDriver = new PGClusterDriver();
        DriverManager.registerDriver(registeredDriver);
        instance = registeredDriver;
    }

    /**
     * According to JDBC specification, this driver is registered against {@link DriverManager} when
     * the class is loaded. To avoid leaks, this method allow unregistering the driver so that the
     * class can be gc'ed if necessary.
     *
     * @throws IllegalStateException if the driver is not registered
     * @throws SQLException          if deregistering the driver fails
     */
    public static void deregister() throws SQLException {
        if (instance == null) {
            throw new IllegalStateException(
                    "Driver is not registered (or it has not been registered using Driver.register() method)");
        }
        DriverManager.deregisterDriver(instance);
        instance = null;
    }

    /**
     * @return {@code true} if the driver is registered against {@link DriverManager}
     */
    public static boolean isRegistered() {
        return instance != null;
    }

    ClusterStatus getClusterStatus(String url, Properties info) throws SQLException {
        if (null == url || !url.startsWith(Urls.JDBC_CLUSTER_PREFIX)) {
            return null;
        }

        Properties properties = org.postgresql.Driver.parseURL(Urls.asPostgresUrl(url),
                null == info ? EMPTY_PROPERTIES : info);
        String normalizedUrl = UrlUtils.getClusterUrl(properties);

        try {
            return CONNECTIONS.computeIfAbsent(normalizedUrl, u -> prepareUrlData(normalizedUrl, properties));
        } catch (RuntimeException e) {
            if (e.getCause() instanceof SQLException) {
                throw (SQLException) e.getCause();
            }
            throw e;
        }
    }

    @Override
    public Connection connect(String url, Properties info) throws SQLException {
        ClusterStatus data = getClusterStatus(url, info);
        if (null == data) {
            return null;
        }
        try {
            return PG_DRIVER.connect(data.getUrl(), info);
        } catch (SQLException e) {
            if (!data.refreshServerStatus()) {
                data.nextUpdate.set(0);
            }
            throw e;
        }
    }

    @Override
    public boolean acceptsURL(String url) throws SQLException {
        return null != url && url.startsWith(Urls.JDBC_CLUSTER_PREFIX) && PG_DRIVER.acceptsURL(Urls.asPostgresUrl(url));
    }

    @Override
    public DriverPropertyInfo[] getPropertyInfo(String url, Properties info) throws SQLException {
        String postgresUrl = Urls.asPostgresUrl(url);
        DriverPropertyInfo[] pgInfo = PG_DRIVER.getPropertyInfo(postgresUrl, info);
        List<DriverPropertyInfo> result = new ArrayList<>(pgInfo.length + PGClusterProperty.values().length);
        result.addAll(Arrays.asList(pgInfo));

        Properties parse = PG_DRIVER.parseURL(postgresUrl, info);
        for (PGClusterProperty p : PGClusterProperty.values()) {
            result.add(p.toDriverPropertyInfo(parse));
        }

        return result.toArray(new DriverPropertyInfo[0]);
    }

    @Override
    public int getMajorVersion() {
        return 0;
    }

    @Override
    public int getMinorVersion() {
        return 0;
    }

    @Override
    public boolean jdbcCompliant() {
        return false;
    }

    @Override
    public Logger getParentLogger() throws SQLFeatureNotSupportedException {
        return PARENT_LOGGER;
    }

    private ClusterStatus prepareUrlData(String url, Properties properties) {
        try {
            ClusterStatus data = new ClusterStatus(url, properties);
            data.refreshServerStatus();
            return data;
        } catch (SQLException e) {
            throw new RuntimeException(e);
        }
    }

    static class ClusterStatus {

        private final String originalUrl;

        private final Properties properties;
        private final String[] serverNames;
        private final String[] portNumbers;
        private final String user;
        private final String password;

        private final String targetServerType;
        private final long hostRecheckPeriod;
        private final boolean loadBalanceHosts;
        private final long maxReplicationLag;

        private volatile ServerStatus[] serverStatuses;
        private volatile long lastUsed;

        private final AtomicLong nextUpdate = new AtomicLong(0);
        private final ReentrantLock lock = new ReentrantLock();

        ClusterStatus(String originalUrl, Properties properties) throws PSQLException {
            this.originalUrl = originalUrl;
            this.properties = properties;

            this.serverNames = PGProperty.PG_HOST.get(properties).split(",");
            this.portNumbers = PGProperty.PG_PORT.get(properties).split(",");
            this.user = PGProperty.USER.get(properties);
            this.password = PGProperty.PASSWORD.get(properties);
            this.targetServerType = PGProperty.TARGET_SERVER_TYPE.get(properties);
            this.hostRecheckPeriod = PGProperty.HOST_RECHECK_SECONDS.getInt(properties) * 1_000;
            this.loadBalanceHosts = PGProperty.LOAD_BALANCE_HOSTS.getBoolean(properties);

            this.maxReplicationLag = PGClusterProperty.MAX_REPLICATION_LAG.getInt(properties);
        }

        String[] getServerNames() {
            return serverNames;
        }

        String[] getPortNumbers() {
            return portNumbers;
        }

        String getTargetServerType() {
            return targetServerType;
        }

        long getHostRecheckPeriod() {
            return hostRecheckPeriod;
        }

        boolean isLoadBalanceHosts() {
            return loadBalanceHosts;
        }

        long getMaxReplicationLag() {
            return maxReplicationLag;
        }

        void setServerStatuses(ServerStatus[] serverStatuses) {
            this.serverStatuses = serverStatuses;
        }

        public String getUrl() throws SQLException {
            lastUsed = System.currentTimeMillis();
            ServerStatus server = getPossibleServer();
            return UrlUtils.getServerUrl(server.getServerName(), server.getPortNumber(), properties);
        }

        protected ServerStatus getPossibleServer() throws SQLException {
            ServerStatus[] allServers = this.serverStatuses;

            switch (targetServerType) {
                case "primary":
                case "master":
                    return getMaster(allServers);

                case "slave":
                case "secondary":
                    Stream<ServerStatus> slaveStream = Arrays.asList(allServers).stream()
                            .filter(this::isPossibleSlave);
                    if (loadBalanceHosts) {
                        return randomServer(allServers, slaveStream.toArray(size -> new ServerStatus[size]));
                    } else {
                        return mostPossibleServer(allServers, slaveStream, NEAREST_HOST_COMPARATOR);
                    }

                case "preferSlave":
                case "preferSecondary":
                    if (loadBalanceHosts) {
                        ServerStatus[] slaves = Arrays.asList(allServers).stream()
                                .filter(this::isPossibleSlave)
                                .toArray(size -> new ServerStatus[size]);
                        if (slaves.length > 0) {
                            return randomServer(slaves);
                        }
                        return getMaster(allServers);
                    } else {
                        Stream<ServerStatus> preferSlaveStream = Arrays.asList(allServers).stream()
                                .filter(this::isPossibleSlaveOrMaster);
                        return mostPossibleServer(allServers, preferSlaveStream, PREFER_SLAVE_COMPARATOR);
                    }

                case "any":
                    Stream<ServerStatus> anyStream = Arrays.asList(allServers).stream()
                            .filter(this::isPossibleSlaveOrMaster);
                    if (loadBalanceHosts) {
                        return randomServer(allServers, anyStream.toArray(size -> new ServerStatus[size]));
                    } else {
                        return mostPossibleServer(allServers, anyStream, NEAREST_HOST_COMPARATOR);
                    }

                default:
                    throw new UnsupportedOperationException(targetServerType);
            }
        }

        private ServerStatus getMaster(ServerStatus[] allServers) {
            return Arrays.asList(allServers).stream()
                    .filter(s -> s.isAvailable() && s.isMaster())
                    .findFirst()
                    .orElseGet(() -> randomServer(allServers));
        }

        private boolean isPossibleSlaveOrMaster(ServerStatus s) {
            return s.isAvailable() && (s.isMaster() || s.getReplicationLag() < maxReplicationLag);
        }

        private boolean isPossibleSlave(ServerStatus s) {
            return s.isAvailable() && !s.isMaster() && s.getReplicationLag() < maxReplicationLag;
        }

        private ServerStatus mostPossibleServer(
                ServerStatus[] allServers,
                Stream<ServerStatus> possibleServersStream,
                Comparator<ServerStatus> cmp
        ) {
            return possibleServersStream
                    .min(cmp)
                    .orElseGet(() -> randomServer(allServers));
        }

        private ServerStatus randomServer(ServerStatus[] allServers, ServerStatus[] possibleServers) {
            return 0 == possibleServers.length ?
                    randomServer(allServers) :
                    randomServer(possibleServers);
        }

        private ServerStatus randomServer(ServerStatus[] serverStatuses) {
            int length = serverStatuses.length;

            if (0 == length) {
                return null;
            }
            if (1 == length) {
                return serverStatuses[0];
            }
            return serverStatuses[ThreadLocalRandom.current().nextInt(length)];
        }

        private boolean refreshServerStatus() {
            if (lock.tryLock()) {
                try {
                    long currentNextUpdate = nextUpdate.get();

                    ServerStatus[] result = new ServerStatus[serverNames.length];
                    for (int i = 0; i < result.length; ++i) {
                        result[i] = getServerStatus(serverNames[i], portNumbers[i]);
                    }

                    long currentTime = System.currentTimeMillis();

                    serverStatuses = result;

                    // Значение может измениться, только если мы принудительно сказали обновить информацию
                    // об этом сервере после получения ошибки соединения (обнулили значение). В этом случае нам надо
                    // максимально быстро перепроверить статус серверов.
                    this.nextUpdate.compareAndSet(currentNextUpdate, currentTime + hostRecheckPeriod);

                    LOGGER.log(Level.INFO, "Server statuses: {0}", Arrays.asList(result));
                } finally {
                    lock.unlock();
                }
                return true;
            }
            return false;
        }

        private ServerStatus getServerStatus(String serverName, String portNumber) {
            try {
                String url = UrlUtils.getServerUrl(serverName, portNumber, properties);
                try (Connection connection = DriverManager.getConnection(url, user, password)) {
                    long startTime = System.currentTimeMillis();
                    try (PreparedStatement statement = connection.prepareStatement(CHECK_QUERY);
                         ResultSet resultSet = statement.executeQuery()) {
                        if (resultSet.next()) {
                            boolean master = !resultSet.getBoolean(1);
                            long replicationLag = master ? Long.MAX_VALUE : resultSet.getLong(2);
                            return new ServerStatus(serverName, portNumber, true, replicationLag,
                                    System.currentTimeMillis() - startTime);
                        } else {
                            return new ServerStatus(serverName, portNumber, false, 0, 0);
                        }
                    }
                }
            } catch (SQLException e) {
                LOGGER.log(Level.WARNING, e.getMessage(), e);
                return new ServerStatus(serverName, portNumber, false, 0, 0);
            }
        }
    }

    private static class RefreshThread extends Thread {

        RefreshThread() {
            setName("PGClusterDriver refresh server status thread");
            setDaemon(true);
        }

        @Override
        public void run() {
            while (true) {
                try {
                    // Запускаем проверку не реже чем раз в 100 мс. т.к. у уже зарегистрированных соединеницй может быть
                    // большое время обновления, и за это время может быть зарегистрированно новое соединение с
                    // маленьким периодом обновления.
                    long nextRefresh = System.currentTimeMillis() + 100;

                    for (var it = CONNECTIONS.entrySet().iterator(); it.hasNext(); ) {
                        Map.Entry<String, ClusterStatus> e = it.next();
                        ClusterStatus data = e.getValue();

                        long currentTime = System.currentTimeMillis();
                        if (data.lastUsed + MAX_UNUSED_PERIOD < currentTime) {
                            it.remove();
                            continue;
                        }

                        if (data.nextUpdate.get() < currentTime) {
                            try {
                                data.refreshServerStatus();
                            } catch (Exception ex) {
                                LOGGER.log(Level.WARNING, "Refresh server status error: ", ex);
                            }
                        }
                        nextRefresh = Math.min(data.nextUpdate.get(), nextRefresh);
                    }
                    long delay = nextRefresh - System.currentTimeMillis();
                    if (0 < delay) {
                        try {
                            Thread.sleep(delay);
                        } catch (InterruptedException e) {
                        }
                    }
                } catch (Throwable e) {
                    LOGGER.log(Level.WARNING, e.getMessage(), e);
                }
            }
        }
    }
}
