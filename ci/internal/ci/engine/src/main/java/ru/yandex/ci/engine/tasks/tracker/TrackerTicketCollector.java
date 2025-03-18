package ru.yandex.ci.engine.tasks.tracker;

import java.time.Instant;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import yav_service.YavOuterClass;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.startrek.client.model.SearchRequest;
import ru.yandex.startrek.client.model.UserRef;

@Slf4j
@RequiredArgsConstructor
public class TrackerTicketCollector {

    private final TrackerSessionSource trackerSessionSource;

    @SuppressWarnings("unchecked")
    public List<TrackerIssue> getIssues(
            YavOuterClass.YavSecretSpec spec,
            YavToken.Id tokenId,
            String queue,
            String status
    ) {
        log.info("Looking up for queue {}, status {}", queue, status);
        var session = trackerSessionSource.getSession(spec, tokenId);

        var statusStartTime = TrackerClient.FIELD_STATUS_START_TIME;
        var request = SearchRequest.builder()
                .filter("queue", queue)
                .filter("status", status)
                .fields(statusStartTime)
                .build();
        var issues = session.issues().find(request).toList();
        log.info("Found {} issues for queue {} and status {}", issues.size(), queue, status);

        return issues.stream()
                .map(issue -> new TrackerIssue(
                        issue.getKey(),
                        issue.getAssignee().map(UserRef::getLogin).orElse((String) null),
                        ((Option<org.joda.time.Instant>) issue.get(statusStartTime))
                                .map(instant -> Instant.ofEpochMilli(instant.getMillis()))
                                .orElseThrow(() -> new RuntimeException("Unable to get field " + statusStartTime))
                ))
                .toList();
    }

    @Value
    public static class TrackerIssue {
        @Nonnull
        String key;
        @Nullable
        String assignee;
        @Nullable
        Instant statusUpdated;
    }
}
