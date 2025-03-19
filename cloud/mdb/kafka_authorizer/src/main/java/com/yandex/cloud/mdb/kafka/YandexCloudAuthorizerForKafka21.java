package com.yandex.cloud.mdb.kafka;

import kafka.network.RequestChannel;
import kafka.security.auth.Operation;
import kafka.security.auth.Resource;
import kafka.security.auth.SimpleAclAuthorizer;
import lombok.extern.slf4j.Slf4j;
import org.apache.kafka.common.acl.AclOperation;
import org.apache.kafka.common.resource.ResourceType;

import java.io.File;
import java.util.Map;

@Slf4j
@SuppressWarnings("deprecation")
public class YandexCloudAuthorizerForKafka21 extends SimpleAclAuthorizer {
    protected final double minFreeSpaceRatio = 0.03;
    protected final String kafkaPartitionDirectoryPath = "/var/lib/kafka";
    protected final File kafkaPartitionDirectory = new File(kafkaPartitionDirectoryPath);

    protected long minFreeSpaceThreshold = 0;

    public YandexCloudAuthorizerForKafka21() {
        super();
    }

    public boolean isEnoughSpace() {
        return kafkaPartitionDirectory.getFreeSpace() >= this.minFreeSpaceThreshold;
    }

    public boolean authorizeByAcl(RequestChannel.Session session, Operation operation, Resource resource) {
        return super.authorize(session, operation, resource);
    }

    @Override
    public void configure(Map<String, ?> javaConfigs) {
        this.minFreeSpaceThreshold = (long) (minFreeSpaceRatio * (double) kafkaPartitionDirectory.getTotalSpace());
        log.info("KafkaAuthorizer: minFreeSpaceThreshold = " + minFreeSpaceThreshold);
        super.configure(javaConfigs);
    }

    @Override
    public boolean authorize(RequestChannel.Session session, Operation operation, Resource resource) {
        boolean result = this.authorizeByAcl(session, operation, resource);
        boolean isEnoughSpace = isEnoughSpace();
        if (isEnoughSpace) {
            return result;
        }
        if (operation.toJava() == AclOperation.WRITE) {
            if (resource.resourceType().toJava() != ResourceType.TOPIC) {
                return result;
            }
            boolean isSpecialTopic = resource.toPattern().name().startsWith("__");
            if (isSpecialTopic) {
                return result;
            }
            // If not enough space left at Kafka local FS partition then deny all write topic actions here
            result = false;
        }
        return result;
    }
}
