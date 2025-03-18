
package ru.yandex.ci.flow.engine.definition.builder;

import java.time.DayOfWeek;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeMap;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.Multimap;
import com.google.common.collect.Sets;
import com.google.protobuf.Message;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.config.a.model.JobSkippedByMode;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.flow.engine.definition.Position;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.common.JobSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.common.ManualTriggerPrompt;
import ru.yandex.ci.flow.engine.definition.common.SchedulerIntervalEntity;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.common.UpstreamStyle;
import ru.yandex.ci.flow.engine.definition.common.UpstreamType;
import ru.yandex.ci.flow.engine.definition.common.WeekSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.job.JobProperties;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;

public class JobBuilderImpl implements JobBuilder {
    private static final Pattern ID_PATTERN = Pattern.compile("[a-zA-Z0-9][0-9a-zA-Z\\-_]*");
    private static final int HOURS_IN_DAY = (int) TimeUnit.DAYS.toHours(1);
    private static final int MINUTES_IN_HOUR = (int) TimeUnit.HOURS.toMinutes(1);

    private final List<Resource> resources;
    private final Set<UpstreamLink<JobBuilder>> upstreams;
    private final String id;
    private final JobType jobType;
    private String title;
    private String description;
    protected JobMultiplyConfig multiply;
    protected boolean manualTrigger;
    protected ManualTriggerPrompt manualTriggerPrompt;
    private final Set<String> tags;
    private CanRunWhen canRunWhen = CanRunWhen.ALL_COMPLETED;
    protected String conditionalRunExpression;
    private Stage stage;
    private boolean shouldHoldPreviousStage;
    private JobAttemptsConfig retry;
    private Position position;
    private InternalExecutorContext internal;
    private TaskletExecutorContext tasklet;
    private TaskletV2ExecutorContext taskletV2;
    private SandboxExecutorContext sandboxTask;
    private JobProperties properties;
    private JobSkippedByMode skippedByMode;

    protected RequirementsConfig requirements;
    protected RuntimeConfig jobRuntimeConfig;
    protected JobSchedulerConstraintEntity jobSchedulerConstraint;

    public JobBuilderImpl(@Nonnull String id, @Nonnull JobType jobType) {
        Preconditions.checkState(!StringUtils.isEmpty(id), "job id cannot be empty");
        Preconditions.checkState(ID_PATTERN.matcher(id).matches(),
                "invalid job id %s, must match %s", id, ID_PATTERN.pattern());
        this.id = id;
        this.jobType = jobType;
        this.resources = new ArrayList<>();
        this.upstreams = new LinkedHashSet<>();
        this.tags = Sets.newHashSet();
    }

    @Override
    public JobBuilder withTitle(String title) {
        this.title = title;
        return this;
    }

    @Override
    public JobBuilder withDescription(String description) {
        this.description = description;
        return this;
    }

    @Override
    public JobBuilder withResources(Resource... resources) {
        Collections.addAll(this.resources, resources);
        return this;
    }

    @Override
    public JobBuilder withResources(List<? extends Resource> resources) {
        this.resources.addAll(resources);
        return this;
    }

    @Override
    public JobBuilder withResources(Message... messages) {
        this.resources.addAll(
                Stream.of(messages)
                        .map(Resource::of)
                        .toList()
        );
        return this;
    }

    @Override
    public JobBuilder withUpstreams(JobBuilder... upstreams) {
        return withUpstreams(UpstreamType.ALL_RESOURCES, upstreams);
    }

    @Override
    public JobBuilder withUpstreams(UpstreamType type, JobBuilder... upstreams) {
        return withUpstreams(type, UpstreamStyle.SPLINE, upstreams);
    }

    @Override
    public JobBuilder withUpstreams(UpstreamType type, UpstreamStyle style, JobBuilder... upstreams) {
        Arrays.stream(upstreams)
                .map(u -> new UpstreamLink<JobBuilder>(u, type, style)).forEach(this.upstreams::add);

        return this;
    }

    @Override
    public JobBuilder withUpstreams(Collection<? extends JobBuilder> upstreams) {
        return withUpstreams(UpstreamType.ALL_RESOURCES, upstreams);
    }

    @Override
    public JobBuilder withUpstreams(UpstreamType type, Collection<? extends JobBuilder> upstreams) {
        return withUpstreams(type, UpstreamStyle.SPLINE, upstreams);
    }

    @Override
    public JobBuilder withUpstreams(
            UpstreamType type,
            UpstreamStyle style,
            Collection<? extends JobBuilder> upstreams
    ) {
        upstreams.stream().map(u -> new UpstreamLink<JobBuilder>(u, type, style)).forEach(this.upstreams::add);
        return this;
    }

    @Override
    public JobBuilder withUpstreams(Multimap<UpstreamType, ? extends JobBuilder> upstreams) {
        upstreams.asMap().forEach(this::withUpstreams);
        return this;
    }

    @Override
    public JobBuilder withUpstreams(CanRunWhen canRunWhen, JobBuilder... upstreams) {
        this.canRunWhen = canRunWhen;
        return withUpstreams(upstreams);
    }

    @Override
    public JobBuilder withUpstreams(CanRunWhen canRunWhen, UpstreamType type, JobBuilder... upstreams) {
        this.canRunWhen = canRunWhen;
        return withUpstreams(type, upstreams);
    }

    @Override
    public JobBuilder withUpstreams(CanRunWhen canRunWhen, List<? extends JobBuilder> upstreams) {
        this.canRunWhen = canRunWhen;
        return withUpstreams(upstreams);
    }

    @Override
    public JobBuilder withUpstreams(CanRunWhen canRunWhen, UpstreamType type, List<? extends JobBuilder> upstreams) {
        this.canRunWhen = canRunWhen;
        return withUpstreams(type, upstreams);
    }

    @Override
    public JobBuilder withRetry(JobAttemptsConfig retry) {
        this.retry = retry;
        return this;
    }

    @Override
    public JobBuilder withCanRunWhen(CanRunWhen canRunWhen) {
        this.canRunWhen = canRunWhen;
        return this;
    }

    @Override
    public JobBuilder withTags(String... tags) {
        Collections.addAll(this.tags, tags);
        return this;
    }

    @Override
    public JobBuilder withTags(List<String> tags) {
        this.tags.addAll(tags);
        return this;
    }

    @Override
    public JobBuilder beginStage(Stage stage) {
        this.stage = stage;
        return this;
    }

    @Override
    public JobBuilder holdPreviousStage() {
        this.shouldHoldPreviousStage = true;
        return this;
    }

    @Override
    public JobBuilder withPosition(Position position) {
        this.position = position;
        return this;
    }

    @Override
    public JobBuilder withInternal(InternalExecutorContext internal) {
        this.internal = internal;
        return this;
    }

    @Override
    public JobBuilder withTasklet(TaskletExecutorContext tasklet) {
        this.tasklet = tasklet;
        return this;
    }

    @Override
    public JobBuilder withTaskletV2(TaskletV2ExecutorContext taskletV2) {
        this.taskletV2 = taskletV2;
        return this;
    }

    @Override
    public JobBuilder withSandboxTask(SandboxExecutorContext sandboxTask) {
        this.sandboxTask = sandboxTask;
        return this;
    }

    @Override
    public String getId() {
        return id;
    }

    @Override
    public String getTitle() {
        return title;
    }

    @Override
    public String getDescription() {
        return description;
    }

    @Override
    public JobMultiplyConfig getMultiply() {
        return multiply;
    }

    @Override
    public boolean hasManualTrigger() {
        return manualTrigger;
    }

    @Override
    public ManualTriggerPrompt getManualTriggerPrompt() {
        return manualTriggerPrompt;
    }

    @Override
    public InternalExecutorContext getInternal() {
        return internal;
    }

    @Override
    public TaskletExecutorContext getTasklet() {
        return tasklet;
    }

    @Override
    public TaskletV2ExecutorContext getTaskletV2() {
        return taskletV2;
    }

    @Override
    public SandboxExecutorContext getSandboxTask() {
        return sandboxTask;
    }

    @Override
    public List<Resource> getResources() {
        return resources;
    }

    @Override
    public Set<UpstreamLink<JobBuilder>> getUpstreams() {
        return upstreams;
    }

    @Override
    public Set<String> getTags() {
        return tags;
    }

    @Override
    public CanRunWhen getCanRunWhen() {
        return canRunWhen;
    }

    @Override
    public String getConditionalRunExpression() {
        return conditionalRunExpression;
    }

    @Override
    public JobAttemptsConfig getRetry() {
        return retry;
    }

    @Override
    public Stage getStage() {
        return stage;
    }

    @Override
    public boolean getShouldHoldPreviousStage() {
        return shouldHoldPreviousStage;
    }

    @Override
    public Position getPosition() {
        return position;
    }

    @Override
    public JobBuilder withSchedulerConstraint(JobSchedulerConstraintEntity schedulerConstraint) {
        this.jobSchedulerConstraint = JobSchedulerConstraintEntity.of(schedulerConstraint);
        return this;
    }

    @Override
    public JobBuilder withMultiply(JobMultiplyConfig template) {
        this.multiply = template;
        return this;
    }

    @Override
    public JobBuilder withManualTrigger() {
        manualTrigger = true;
        return this;
    }

    @Override
    public JobBuilder withPrompt(String question) {
        manualTrigger = true;
        manualTriggerPrompt = new ManualTriggerPrompt(question);
        return this;
    }

    @Override
    public JobBuilder withPrompt() {
        manualTrigger = true;
        manualTriggerPrompt = new ManualTriggerPrompt(null);
        return this;
    }

    @Override
    public JobBuilder withConditionalRunExpression(String expression) {
        conditionalRunExpression = expression;
        return this;
    }

    @Override
    public JobBuilder withRequirements(RequirementsConfig requirements) {
        this.requirements = requirements;
        return this;
    }

    @Override
    public JobBuilder withJobRuntimeConfig(@Nullable RuntimeConfig jobRuntimeConfig) {
        this.jobRuntimeConfig = jobRuntimeConfig;
        return this;
    }

    @Override
    public JobSchedulerBuilder withScheduler() {
        return new JobSchedulerBuilderImpl();
    }

    @Override
    public JobSchedulerConstraintEntity getJobSchedulerConstraint() {
        return jobSchedulerConstraint;
    }

    @Override
    public RequirementsConfig getRequirements() {
        return requirements;
    }

    @Override
    public RuntimeConfig getJobRuntimeConfig() {
        return jobRuntimeConfig;
    }

    @Override
    public JobType getJobType() {
        return jobType;
    }

    @Override
    public JobBuilder withProperties(JobProperties properties) {
        this.properties = properties;
        return this;
    }

    @Override
    public JobProperties getProperties() {
        return properties;
    }

    @Override
    public JobBuilder withSkippedByMode(JobSkippedByMode skippedByMode) {
        this.skippedByMode = skippedByMode;
        return this;
    }

    @Override
    public JobSkippedByMode getSkippedByMode() {
        return skippedByMode;
    }

    @Override
    public String toString() {
        return "JobBuilder{" +
                (id != null ? (", id='" + id + '\'') : "") +
                (title != null ? (", title='" + title + '\'') : "") +
                '}';
    }


    private JobBuilder withScheduler(JobSchedulerConstraintEntity schedulerConstraint) {
        this.jobSchedulerConstraint = JobSchedulerConstraintEntity.of(schedulerConstraint);
        return this;
    }

    public class JobSchedulerBuilderImpl implements JobSchedulerBuilder {
        private final TreeMap<TypeOfSchedulerConstraint, WeekSchedulerConstraintEntity> weekConstraints;

        protected JobSchedulerBuilderImpl() {
            this.weekConstraints = new TreeMap<>();
        }

        @Override
        public JobSchedulerBuilder workDaysHours(int hoursFrom, int hoursTo) {
            return this.workDays(hoursFrom, 0, hoursTo, 0);
        }

        @Override
        public JobSchedulerBuilder workDaysHours(DayOfWeek dayOfWeek, int hoursFrom, int hoursTo) {
            return this.workDays(dayOfWeek, hoursFrom, 0, hoursTo, 0);
        }

        @Override
        public JobSchedulerBuilder workDays(int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            this.weekConstraint(TypeOfSchedulerConstraint.WORK, hoursFrom, minutesFrom, hoursTo, minutesTo);
            return this;
        }

        @Override
        public JobSchedulerBuilder workDays(DayOfWeek dayOfWeek,
                                            int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            this.dayOfWeekConstraint(dayOfWeek, TypeOfSchedulerConstraint.WORK,
                    hoursFrom, minutesFrom, hoursTo, minutesTo);
            return this;
        }

        @Override
        public JobSchedulerBuilder preHolidayHours(int hoursFrom, int hoursTo) {
            return this.preHoliday(hoursFrom, 0, hoursTo, 0);
        }

        @Override
        public JobSchedulerBuilder preHolidayHours(DayOfWeek dayOfWeek, int hoursFrom, int hoursTo) {
            return this.preHoliday(hoursFrom, 0, hoursTo, 0);
        }

        @Override
        public JobSchedulerBuilder preHoliday(int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            this.weekConstraint(TypeOfSchedulerConstraint.PRE_HOLIDAY, hoursFrom, minutesFrom, hoursTo, minutesTo);
            return this;
        }

        @Override
        public JobSchedulerBuilder preHoliday(DayOfWeek dayOfWeek,
                                              int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            this.dayOfWeekConstraint(dayOfWeek, TypeOfSchedulerConstraint.PRE_HOLIDAY,
                    hoursFrom, minutesFrom, hoursTo, minutesTo);
            return this;
        }

        @Override
        public JobSchedulerBuilder holidayHours(int hoursFrom, int hoursTo) {
            return this.holiday(hoursFrom, 0, hoursTo, 0);
        }

        @Override
        public JobSchedulerBuilder holidayHours(DayOfWeek dayOfWeek, int hoursFrom, int hoursTo) {
            return this.holiday(dayOfWeek, hoursFrom, 0, hoursTo, 0);
        }

        @Override
        public JobSchedulerBuilder holiday(int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            this.weekConstraint(TypeOfSchedulerConstraint.HOLIDAY, hoursFrom, minutesFrom, hoursTo, minutesTo);
            return this;
        }

        @Override
        public JobSchedulerBuilder holiday(DayOfWeek dayOfWeek,
                                           int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            this.dayOfWeekConstraint(dayOfWeek, TypeOfSchedulerConstraint.HOLIDAY,
                    hoursFrom, minutesFrom, hoursTo, minutesTo);
            return this;
        }

        private void weekConstraint(TypeOfSchedulerConstraint type,
                                    int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            if (weekConstraints.containsKey(type)) {
                throw new RuntimeException("WeekConstraints already exists for type " + type.name());
            }
            int totalMinutesFrom = getTotalMinutes(hoursFrom, minutesFrom);
            int totalMinutesTo = getTotalMinutes(hoursTo, minutesTo);

            weekConstraints.put(type,
                    WeekSchedulerConstraintEntity.of(
                            new SchedulerIntervalEntity(totalMinutesFrom, totalMinutesTo)
                    ));
        }

        private void dayOfWeekConstraint(DayOfWeek dayOfWeek, TypeOfSchedulerConstraint type,
                                         int hoursFrom, int minutesFrom, int hoursTo, int minutesTo) {
            int totalMinutesFrom = getTotalMinutes(hoursFrom, minutesFrom);
            int totalMinutesTo = getTotalMinutes(hoursTo, minutesTo);

            WeekSchedulerConstraintEntity weekConstraint = weekConstraints.get(type);
            if (weekConstraint == null) {
                weekConstraint = WeekSchedulerConstraintEntity.of();
                weekConstraints.put(type, weekConstraint);
            }

            weekConstraint.addAllowedDayOfWeek(dayOfWeek,
                    new SchedulerIntervalEntity(totalMinutesFrom, totalMinutesTo)
            );
        }

        private int getTotalMinutes(int hours, int minutes) {
            if (hours < 0
                    || hours > HOURS_IN_DAY
                    || (hours >= HOURS_IN_DAY && minutes > 0)
                    || minutes < 0
                    || minutes >= MINUTES_IN_HOUR) {
                throw new RuntimeException(String.format("Incorrect scheduler value: %d:%d", hours, minutes));
            }
            return (int) TimeUnit.HOURS.toMinutes(hours) + minutes;
        }

        @Override
        public JobBuilder build() {
            return JobBuilderImpl.this.withScheduler(new JobSchedulerConstraintEntity(weekConstraints));
        }
    }
}
