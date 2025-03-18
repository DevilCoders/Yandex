package ru.yandex.ci.tms.metric.ci;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import io.micrometer.core.instrument.Tag;

import ru.yandex.ci.ydb.service.metric.MetricId;

public class CiSystemUsageMetricIdBuilder {

    private final String name;

    @Nullable
    private Tag systemTag;

    @Nullable
    private Tag typeTag;

    @Nullable
    private Tag vcsTag;

    @Nullable
    private Tag windowTag;

    @Nullable
    private Tag statusTag;

    private CiSystemUsageMetricIdBuilder(String name) {
        this.name = name;
    }

    public MetricId build() {
        Preconditions.checkState(systemTag != null);
        Preconditions.checkState(statusTag != null);
        Preconditions.checkState(typeTag != null);
        Preconditions.checkState(vcsTag != null);
        Preconditions.checkState(windowTag != null);
        return MetricId.of(name, systemTag, statusTag, typeTag, vcsTag, windowTag);
    }

    public static CiSystemUsageMetricIdBuilder createActive() {
        return new CiSystemUsageMetricIdBuilder("ci_system_usage_active");
    }

    private CiSystemUsageMetricIdBuilder withSystem(String system) {
        systemTag = Tag.of(CiSystemsUsageMetrics.SYSTEM_TAG_NAME, system);
        return this;
    }

    public CiSystemUsageMetricIdBuilder withTrendbox() {
        return withSystem("Trendbox");
    }

    private CiSystemUsageMetricIdBuilder withVcs(String vcs) {
        vcsTag = Tag.of("vcs", vcs);
        return this;
    }

    public CiSystemUsageMetricIdBuilder withTotalVcs() {
        return withVcs("total");
    }

    public CiSystemUsageMetricIdBuilder withArcadia() {
        return withVcs("arcadia");
    }

    public CiSystemUsageMetricIdBuilder withGithub() {
        return withVcs("github");
    }

    public CiSystemUsageMetricIdBuilder withBitbucket() {
        return withVcs("bitbucket");
    }

    private CiSystemUsageMetricIdBuilder withWindowsDays(int days) {
        windowTag = Tag.of("windowDays", String.valueOf(days));
        return this;
    }

    public CiSystemUsageMetricIdBuilder withDayWindow() {
        return withWindowsDays(1);
    }

    public CiSystemUsageMetricIdBuilder withWeekWindow() {
        return withWindowsDays(7);
    }

    public CiSystemUsageMetricIdBuilder with2WeekWindow() {
        return withWindowsDays(14);
    }

    public CiSystemUsageMetricIdBuilder withMonthWindow() {
        return withWindowsDays(30);
    }

    public CiSystemUsageMetricIdBuilder withFlowsType() {
        typeTag = CiSystemsUsageMetrics.FLOWS_TAG;
        return this;
    }

    private CiSystemUsageMetricIdBuilder withStatus(String status) {
        statusTag = Tag.of("status", status);
        return this;
    }

    public CiSystemUsageMetricIdBuilder withAllStatuses() {
        return withStatus("all");
    }

    public CiSystemUsageMetricIdBuilder withSuccessStatus() {
        return withStatus("success");
    }
}
