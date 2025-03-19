package com.yandex.cloud.mdb.kafka;

import kafka.security.auth.Operation;
import kafka.security.auth.Resource;
import org.apache.kafka.common.acl.AclOperation;
import org.apache.kafka.common.resource.PatternType;
import org.apache.kafka.common.resource.ResourceType;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SuppressWarnings( "deprecation" )
class YandexCloudAuthorizerForKafka21Test {

    @Test
    void authorize() {
        YandexCloudAuthorizerForKafka21 yandexCloudAuthorizerMock = Mockito.spy(new YandexCloudAuthorizerForKafka21());
        Mockito.doReturn(true)
                .when(yandexCloudAuthorizerMock)
                .authorizeByAcl(Mockito.any(), Mockito.any(), Mockito.any());
        Mockito.when(yandexCloudAuthorizerMock.isEnoughSpace()).thenReturn(true, false, false);

        Operation operation = Operation.fromJava(AclOperation.WRITE);
        Resource resource = new Resource(
                kafka.security.auth.ResourceType.fromJava(ResourceType.TOPIC),
                "test_topic",
                PatternType.LITERAL
        );
        assertTrue(yandexCloudAuthorizerMock.authorize(null, operation, resource));
        assertFalse(yandexCloudAuthorizerMock.authorize(null, operation, resource));

        resource = new Resource(
                kafka.security.auth.ResourceType.fromJava(ResourceType.TOPIC),
                "__consumer_offsets",
                PatternType.LITERAL
        );
        assertTrue(yandexCloudAuthorizerMock.authorize(null, operation, resource));
    }
}
