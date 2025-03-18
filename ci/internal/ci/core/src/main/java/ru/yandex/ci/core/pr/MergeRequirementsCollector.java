package ru.yandex.ci.core.pr;

import java.util.Set;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;

public interface MergeRequirementsCollector {

    void add(ArcanumMergeRequirementId mergeRequirement);

    Set<ArcanumMergeRequirementId> getMergeRequirements();

    static MergeRequirementsCollector create() {
        return new MergeRequirementsCollectorImpl();
    }

    static MergeRequirementsCollector dummy() {
        return new MergeRequirementsCollector() {
            @Override
            public void add(ArcanumMergeRequirementId mergeRequirement) {
            }

            @Override
            public Set<ArcanumMergeRequirementId> getMergeRequirements() {
                return Set.of();
            }
        };
    }

}
