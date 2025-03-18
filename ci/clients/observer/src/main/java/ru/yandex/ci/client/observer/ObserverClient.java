package ru.yandex.ci.client.observer;

import java.time.Duration;
import java.util.List;

import ru.yandex.ci.client.base.http.RetryPolicies;
import ru.yandex.ci.client.base.http.RetryPolicy;

public interface ObserverClient {

    static RetryPolicy defaultRetryPolicy() {
        return RetryPolicies.retryWithSleep(10, Duration.ofSeconds(1));
    }

    List<CheckRevisionsDto> getNotUsedRevisions(String startRevision,
                                                String duration,
                                                int revisionsPerHour,
                                                String namespace);

    void markAsUsed(String rightRevision, String leftRevision, String namespace, String flowLaunchId);

    List<UsedRevisionResponseDto> findUsedRevisions(List<String> rightRevisions, String namespace);
}
