package ru.yandex.ci.core.timeline;

import org.assertj.core.api.ObjectAssert;

class TimelineItemAssert extends ObjectAssert<TimelineItem> {

    TimelineItemAssert(TimelineItem timelineItem) {
        super(timelineItem);
    }

    static TimelineItemAssert assertItem(TimelineItem actual) {
        return new TimelineItemAssert(actual);
    }

    public TimelineItemAssert hasBranch(String branchName) {
        doesntHaveLaunch();
        if (actual.getBranch() == null) {
            failWithMessage("expected branch %s, but branch is null; item: %s", branchName, actual);
            return this;
        }
        if (!branchName.equals(actual.getBranch().getId().getBranch())) {
            failWithMessage("expected branch %s, but was %s; item: %s",
                    branchName, actual.getBranch().getId().getBranch(), actual);
        }
        return this;
    }

    public TimelineItemAssert hasLaunch(int launchNumber) {
        doesntHaveBranch();
        if (actual.getLaunch() == null) {
            failWithMessage("expected launch %s, but launch is null", launchNumber);
            return this;
        }
        int actualNumber = actual.getLaunch().getLaunchId().getNumber();
        if (actualNumber != launchNumber) {
            failWithMessage("expected launch %s, but was %s; item: %s", launchNumber, actualNumber, actual);
        }
        return this;
    }

    public TimelineItemAssert hasRev(long revision) {
        if (actual.getRevision() != revision) {
            failWithMessage("expected revision %s, but was %s; item: %s", revision, actual.getRevision(), actual);
        }
        return this;
    }

    public TimelineItemAssert doesntHaveLaunch() {
        if (actual.getLaunch() != null) {
            failWithMessage("expected do not have launch, but has %s; item: %s", actual.getLaunch(), actual);
        }
        return this;
    }

    public TimelineItemAssert doesntHaveBranch() {
        if (actual.getBranch() != null) {
            failWithMessage("expected do not have branch, but has %s; item: %s",
                    actual.getBranch().getId().getBranch(), actual);
        }
        return this;
    }

}
