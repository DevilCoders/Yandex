package ru.yandex.ds.pgdriver;

class ServerStatus {
    private final String serverName;
    private final String portNumber;
    private final boolean available;
    private final long replicationLag;
    private final long pingTime;

    ServerStatus(
            String serverName,
            String portNumber,
            boolean available,
            long replicationLag,
            long pingTime
    ) {
        this.serverName = serverName;
        this.portNumber = portNumber;
        this.available = available;
        this.replicationLag = replicationLag;
        this.pingTime = pingTime;
    }

    public String getServerName() {
        return serverName;
    }

    public String getPortNumber() {
        return portNumber;
    }

    public boolean isAvailable() {
        return available;
    }

    public long getReplicationLag() {
        return replicationLag;
    }

    public long getPingTime() {
        return pingTime;
    }

    boolean isMaster() {
        return Long.MAX_VALUE == replicationLag;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder(100);
        sb.append("ServerStatus{")
                .append(serverName).append(':').append(portNumber).append(", ");

        if (available) {
            sb.append("AVAILABLE");
            if (isMaster()) {
                sb.append(", MASTER");
            } else {
                sb.append(", SLAVE, LAG=").append(replicationLag);
            }
            sb.append(", PING=").append(pingTime);
        } else {
            sb.append("DOWN");
        }
        sb.append("}");

        return sb.toString();
    }
}
