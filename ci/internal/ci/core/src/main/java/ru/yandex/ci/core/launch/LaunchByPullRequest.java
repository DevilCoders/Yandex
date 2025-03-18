package ru.yandex.ci.core.launch;

import java.util.Optional;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Table(name = "main/LaunchByPullRequest")
public class LaunchByPullRequest implements Entity<LaunchByPullRequest> {

    LaunchByPullRequest.Id id;

    @Override
    public LaunchByPullRequest.Id getId() {
        return id;
    }

    public static Optional<LaunchByPullRequest> of(Launch launch) {
        var pr = launch.getVcsInfo().getPullRequestInfo();
        if (pr == null) {
            return Optional.empty();
        } else {
            var id = Id.of(pr.getPullRequestId(), pr.getDiffSetId(), launch.getId());
            return Optional.of(new LaunchByPullRequest(id));
        }
    }


    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<LaunchByPullRequest> {

        @Column(name = "idx_pullRequestId")
        long pullRequestId;

        @Column(name = "idx_diffSetId")
        long diffSetId;

        Launch.Id id;
    }
}
