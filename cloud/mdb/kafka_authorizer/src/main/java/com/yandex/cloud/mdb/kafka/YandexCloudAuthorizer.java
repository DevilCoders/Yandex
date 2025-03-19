package com.yandex.cloud.mdb.kafka;

import kafka.security.authorizer.AclAuthorizer;
import kafka.metrics.KafkaYammerMetrics;
import lombok.extern.slf4j.Slf4j;
import org.apache.kafka.common.Endpoint;
import org.apache.kafka.common.acl.AclOperation;
import org.apache.kafka.common.resource.ResourcePattern;
import org.apache.kafka.common.resource.ResourceType;
import org.apache.kafka.server.authorizer.Action;
import org.apache.kafka.server.authorizer.AuthorizableRequestContext;
import org.apache.kafka.server.authorizer.AuthorizationResult;
import org.apache.kafka.server.authorizer.AuthorizerServerInfo;
import com.yammer.metrics.core.Gauge;
import com.yammer.metrics.core.MetricName;

import java.io.File;
import java.time.Duration;
import java.time.LocalDateTime;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.concurrent.CompletionStage;


@Slf4j
public class YandexCloudAuthorizer extends AclAuthorizer {

    protected final double minFreeSpaceRatio = 0.03;
    protected final String kafkaPartitionDirectoryPath = "/var/lib/kafka";
    protected final File kafkaPartitionDirectory = new File(kafkaPartitionDirectoryPath);
    private LocalDateTime lastReportDatetime = LocalDateTime.now();
    private boolean isEnoughSpaceOnDisk;

    protected long minFreeSpaceThreshold = 0;

    public List<AuthorizationResult> authorizeByAcl(AuthorizableRequestContext requestContext, List<Action> actions) {
        return super.authorize(requestContext, actions);
    }

    public void registerMetric() {
        Gauge<Integer> isEnoughSpaceOnDiskGauge = new Gauge<Integer>() {
            public Integer value() {
                return isEnoughSpaceOnDisk ? 1 : 0;
            }
        };
        MetricName name = new MetricName("kafka.authorizer", "BrokerStatus", "EnoughSpace", null, "kafka.authorizer:type=BrokerStatus,name=EnoughSpace");
        KafkaYammerMetrics.defaultRegistry().newGauge(name, isEnoughSpaceOnDiskGauge);
    }

    public Map<Endpoint, ? extends CompletionStage<Void>> start(AuthorizerServerInfo serverInfo) {
        minFreeSpaceThreshold = (long) (minFreeSpaceRatio * (double) kafkaPartitionDirectory.getTotalSpace());
        log.info("KafkaAuthorizer: minFreeSpaceThreshold = " + minFreeSpaceThreshold);
        registerMetric();
        return super.start(serverInfo);
    }

    private static String getSizeInGB(long size) {
        long GB = 1024 * 1024 * 1024;
        return String.format("%.2f", (double)size / GB) + " GB";
    }

    public boolean isEnoughSpace() {
        LocalDateTime date = LocalDateTime.now();
        long freeSpace = kafkaPartitionDirectory.getFreeSpace();
        isEnoughSpaceOnDisk = freeSpace >= minFreeSpaceThreshold;

        // Report about current status no more than once in a minute
        long secondsPassed = Duration.between(lastReportDatetime, date).getSeconds();
        if (secondsPassed > 60) {
            log.info("KafkaAuthorizer: stats = free: {} ({}) threshold: {} ({}) is enough: {}",
                freeSpace, getSizeInGB(freeSpace),
                minFreeSpaceThreshold, getSizeInGB(minFreeSpaceThreshold),
                isEnoughSpaceOnDisk);
            lastReportDatetime = date;
        }

        return isEnoughSpaceOnDisk;
    }

    public List<AuthorizationResult> authorize(AuthorizableRequestContext requestContext, List<Action> actions) {
        List<AuthorizationResult> results = authorizeByAcl(requestContext, actions);
        ListIterator<Action> it = actions.listIterator();
        if (isEnoughSpace()) {
            return results;
        }
        while (it.hasNext()) {
            Action action = it.next();
            int actionIndex = it.previousIndex();

            if (action.operation() == AclOperation.WRITE) {
                ResourcePattern resourcePattern = action.resourcePattern();
                if (resourcePattern.resourceType() != ResourceType.TOPIC) {
                    continue;
                }
                boolean isSpecialTopic = resourcePattern.name().startsWith("__");
                if (isSpecialTopic) {
                    continue;
                }
                // If not enough space left at Kafka local FS partition then deny all write topic actions here
                results.set(actionIndex, AuthorizationResult.DENIED);
            }
        }
        return results;
    }
}
