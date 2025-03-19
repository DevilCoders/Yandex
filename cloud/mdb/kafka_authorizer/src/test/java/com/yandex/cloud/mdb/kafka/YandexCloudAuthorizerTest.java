package com.yandex.cloud.mdb.kafka;

import org.apache.kafka.common.acl.AclOperation;
import org.apache.kafka.common.message.RequestHeaderData;
import org.apache.kafka.common.requests.RequestContext;
import org.apache.kafka.common.requests.RequestHeader;
import org.apache.kafka.common.resource.PatternType;
import org.apache.kafka.common.resource.ResourcePattern;
import org.apache.kafka.common.resource.ResourceType;
import org.apache.kafka.common.security.auth.KafkaPrincipal;
import org.apache.kafka.server.authorizer.Action;
import org.apache.kafka.server.authorizer.AuthorizableRequestContext;
import org.apache.kafka.server.authorizer.AuthorizationResult;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

class YandexCloudAuthorizerTest {
    private InetAddress getInetAddress() {
        try {
            return InetAddress.getByName("8.8.8.8");
        } catch (UnknownHostException e) {
        }
        return null;
    }

    @Test
    void authorize() {
        AuthorizableRequestContext requestContext = new RequestContext(
                new RequestHeader(new RequestHeaderData(), (short) 0),
                null,
                getInetAddress(),
                new KafkaPrincipal("User", "user"),
                null,
                null,
                null
        );
        Action action = new Action(
                AclOperation.WRITE,
                new ResourcePattern(ResourceType.TOPIC, "test_topic", PatternType.LITERAL),
                1,
                true,
                true
        );
        ArrayList<Action> actions = new ArrayList<>();
        actions.add(action);

        YandexCloudAuthorizer yandexCloudAuthorizerMock = Mockito.spy(new YandexCloudAuthorizer());
        ArrayList<AuthorizationResult> aclAuthorizerResults = new ArrayList<>();
        aclAuthorizerResults.add(AuthorizationResult.ALLOWED);

        Mockito.doReturn(aclAuthorizerResults)
                .when(yandexCloudAuthorizerMock)
                .authorizeByAcl(Mockito.any(), Mockito.any());
        Mockito.when(yandexCloudAuthorizerMock.isEnoughSpace()).thenReturn(true, false, false);

        List<AuthorizationResult> results = yandexCloudAuthorizerMock.authorize(requestContext, actions);
        assertEquals(AuthorizationResult.ALLOWED, results.get(0));

        results = yandexCloudAuthorizerMock.authorize(requestContext, actions);
        assertEquals(AuthorizationResult.DENIED, results.get(0));

        aclAuthorizerResults.set(0, AuthorizationResult.ALLOWED);
        actions.set(0, new Action(
                AclOperation.WRITE,
                new ResourcePattern(ResourceType.TOPIC, "__consumer_offsets", PatternType.LITERAL),
                1,
                true,
                true
        ));
        results = yandexCloudAuthorizerMock.authorize(requestContext, actions);
        assertEquals(AuthorizationResult.ALLOWED, results.get(0));
    }
}