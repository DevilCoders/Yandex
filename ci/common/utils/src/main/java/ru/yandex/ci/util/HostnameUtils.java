package ru.yandex.ci.util;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class HostnameUtils {
    private static final String HOSTNAME;
    private static final String SHORT_HOSTNAME;

    static {
        try {
            HOSTNAME = InetAddress.getLocalHost().getCanonicalHostName();
            SHORT_HOSTNAME = HOSTNAME.replace(".yp-c.yandex.net", "");
        } catch (UnknownHostException e) {
            throw new RuntimeException(e);
        }
    }

    private HostnameUtils() {

    }

    public static String getHostname() {
        return HOSTNAME;
    }

    public static String getShortHostname() {
        return SHORT_HOSTNAME;
    }
}
