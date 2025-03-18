package ru.yandex.ci.client.oldci;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class CiCheck {
    @Nullable
    String id;
    @Nullable
    String status;
    int finishTimestamp;
    int startTimestamp;
    @Nullable
    Boolean pessimized;
    @Nullable
    Progress progress;
    @Nullable
    Statistics statistics;
    @Nullable
    ExtendedStatistic extendedStatistics;

    @Value
    public static class Statistics {
        @Nullable
        Stat configure;
        @Nullable
        Stat build;
        @Nullable
        Stat style;
        @Nullable
        Stat small;
        @Nullable
        Stat medium;
        @Nullable
        Stat large;
        @Nullable
        Stat teJob;
    }

    @Value
    public static class Progress {
        @Nullable
        String status;
        @Nullable
        String checkStatus;
    }

    @Value
    public static class Stat {
        boolean finished;
        int broken;
    }

    @Value
    public static class ExtendedStatistic {
        @Nullable
        Added added;
    }

    @Value
    public static class Added {
        int total;
    }
}
