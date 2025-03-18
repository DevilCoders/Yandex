package ru.yandex.ci.core.pr;

import java.util.HashSet;
import java.util.Set;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;

class MergeRequirementsCollectorImpl implements MergeRequirementsCollector {

    private final Set<ArcanumMergeRequirementId> requiredMergeRequirements = new HashSet<>();

    @Override
    public void add(ArcanumMergeRequirementId mergeRequirement) {
        requiredMergeRequirements.add(mergeRequirement);
    }

    @Override
    public Set<ArcanumMergeRequirementId> getMergeRequirements() {
        return requiredMergeRequirements;
    }

}
