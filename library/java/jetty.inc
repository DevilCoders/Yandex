SET(jetty_version 9.4.43.v20210629)

DEPENDENCY_MANAGEMENT(
    contrib/java/org/eclipse/jetty/jetty-annotations/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-client/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-continuation/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-http/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-io/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-jaas/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-jmx/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-jndi/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-plus/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-proxy/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-runner/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-security/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-server/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-servlet/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-servlets/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-util/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-webapp/${jetty_version}
    contrib/java/org/eclipse/jetty/jetty-xml/${jetty_version}

    contrib/java/org/eclipse/jetty/websocket/javax-websocket-client-impl/${jetty_version}
    contrib/java/org/eclipse/jetty/websocket/javax-websocket-server-impl/${jetty_version}
    contrib/java/org/eclipse/jetty/websocket/websocket-api/${jetty_version}
    contrib/java/org/eclipse/jetty/websocket/websocket-client/${jetty_version}
    contrib/java/org/eclipse/jetty/websocket/websocket-common/${jetty_version}
    contrib/java/org/eclipse/jetty/websocket/websocket-server/${jetty_version}
    contrib/java/org/eclipse/jetty/websocket/websocket-servlet/${jetty_version}
)
