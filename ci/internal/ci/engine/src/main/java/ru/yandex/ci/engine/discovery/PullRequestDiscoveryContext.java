package ru.yandex.ci.engine.discovery;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.pr.MergeRequirementsCollector;
import ru.yandex.ci.core.pr.PullRequestDiffSet;

public class PullRequestDiscoveryContext implements MergeRequirementsCollector {

    @Nonnull
    private final PullRequestDiffSet diffSet;
    private final List<LaunchId> createdLaunches = new ArrayList<>();
    private final MergeRequirementsCollector mergeRequirementsCollector = MergeRequirementsCollector.create();

    public PullRequestDiscoveryContext(PullRequestDiffSet diffSet) {
        this.diffSet = diffSet;
    }

    public PullRequestDiffSet getDiffSet() {
        return diffSet;
    }

    @Override
    public void add(ArcanumMergeRequirementId mergeRequirement) {
        mergeRequirementsCollector.add(mergeRequirement);
    }

    @Override
    public Set<ArcanumMergeRequirementId> getMergeRequirements() {
        return mergeRequirementsCollector.getMergeRequirements();
    }

    public void addCreatedLaunch(LaunchId launchId) {
        createdLaunches.add(launchId);
    }

    public List<LaunchId> getCreatedLaunches() {
        return createdLaunches;
    }
}
