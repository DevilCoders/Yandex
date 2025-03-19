import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import javax.management.AttributeNotFoundException;
import javax.management.InstanceNotFoundException;
import javax.management.MBeanException;
import javax.management.MBeanServerConnection;
import javax.management.MalformedObjectNameException;
import javax.management.ObjectName;
import javax.management.ReflectionException;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;

import org.apache.kafka.clients.admin.Admin;
import org.apache.kafka.clients.admin.AdminClient;
import py4j.GatewayServer;


public class KafkaAdmin {
    private JMXConnector jmxConnector;
    private MBeanServerConnection mBeanServerConnection;

    public Admin admin(Properties props) {
        return AdminClient.create(props);
    }

    public static void main(String[] args) {
        KafkaAdmin app = new KafkaAdmin();
        // app is now the gateway.entry_point
        GatewayServer server = new GatewayServer(app);
        server.start();
    }

    public static class GetMetricResponse {
        @SuppressWarnings("VisibilityModifier")
        public String value;
        @SuppressWarnings("VisibilityModifier")
        public int errorCode;
        @SuppressWarnings("VisibilityModifier")
        public String errorMessage;

        public GetMetricResponse(String value, int errorCode, String errorMessage) {
            this.value = value;
            this.errorCode = errorCode;
            this.errorMessage = errorMessage;
        }

        public GetMetricResponse(int errorCode, String errorMessage) {
            this.value = "";
            this.errorCode = errorCode;
            this.errorMessage = errorMessage;
        }
    }

    @SuppressWarnings("MethodName")
    public GetMetricResponse get_metric(String metricName) {
        MBeanServerConnection mbsc;
        try {
            mbsc = getMbeanServerConnection();
        } catch (IOException e) {
            return new GetMetricResponse(1, "Failed to connect to JMX server: " + e.getMessage());
        }

        ObjectName objectName;
        try {
            objectName = new ObjectName(metricName);
        } catch (MalformedObjectNameException e) {
            return new GetMetricResponse(9, "Invalid object name: " + e.getMessage());
        }

        Object value;
        try {
            value = mbsc.getAttribute(objectName, "Value");
        } catch (MBeanException | ReflectionException | IOException e) {
            return new GetMetricResponse(9, "Failed to get metric \"" + metricName + "\": " + e.getMessage());
        } catch (AttributeNotFoundException e) {
            return new GetMetricResponse(3,
                "Object \"" + metricName + "\" does not have attribute with name \"Value\"");
        } catch (InstanceNotFoundException e) {
            return new GetMetricResponse(2, "Object \"" + metricName + "\" not found");
        }

        return new GetMetricResponse(value.toString(), 0, "");
    }


    private MBeanServerConnection getMbeanServerConnection() throws IOException {
        if (mBeanServerConnection != null) {
            try {
                mBeanServerConnection.getMBeanCount();
            } catch (IOException e) {
                System.err.println("MBean server connection is stale: " + e.getMessage());
                jmxConnector = null;
                mBeanServerConnection = null; // then try to reconnect
            }
        }

        if (mBeanServerConnection == null) {
            if (jmxConnector == null) {
                JMXServiceURL url = new JMXServiceURL("service:jmx:rmi:///jndi/rmi://127.0.0.1:9999/jmxrmi");
                Map<String, Object> env = new HashMap<>();
                String pair = System.getenv().get("JMX_AUTH");
                if (pair != null) {
                    String[] credentials = pair.split(" ", 2);
                    env.put(JMXConnector.CREDENTIALS, credentials);
                }
                jmxConnector = JMXConnectorFactory.connect(url, env);
            }
            mBeanServerConnection = jmxConnector.getMBeanServerConnection();
        }

        return mBeanServerConnection;
    }
}
