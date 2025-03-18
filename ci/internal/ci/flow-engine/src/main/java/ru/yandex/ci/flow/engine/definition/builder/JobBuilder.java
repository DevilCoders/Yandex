package ru.yandex.ci.flow.engine.definition.builder;

import java.time.DayOfWeek;
import java.util.Collection;
import java.util.List;
import java.util.Set;

import com.google.common.collect.Multimap;
import com.google.protobuf.Message;

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
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.common.UpstreamStyle;
import ru.yandex.ci.flow.engine.definition.common.UpstreamType;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.job.JobProperties;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;

public interface JobBuilder {

    JobBuilder withTitle(String title);

    JobBuilder withDescription(String description);

    JobBuilder withMultiply(JobMultiplyConfig multiply);

    /**
     * Бывают ресурсы, которые джоба не может получить из апстримов. Как правило,
     * это конфигурация, которая является неотъмлемым свойством джобы. Например,
     * для джобы деплоя в кондуктор таким свойством является кондукторная ветвь:
     * TESTING, PRESTABLE, etc.
     *
     * @param resources список ресурсов
     * @return <T> - a JobBuilder
     */
    JobBuilder withResources(Resource... resources);

    /**
     * Включение списка ресурсов
     *
     * @param resources список ресурсов
     * @return <T> - a JobBuilder
     */
    JobBuilder withResources(List<? extends Resource> resources);

    /**
     * Включение списка ресурсов из Proto сообщений
     *
     * @param messages список сообщений
     * @return <T> - a JobBuilder
     */
    JobBuilder withResources(Message... messages);

    /**
     * Определяет соедининия ({@link Job}), от которых зависит текущая джоба.
     *
     * @param upstreams список апстримов
     * @return <T> - a JobBuilder
     */
    JobBuilder withUpstreams(JobBuilder... upstreams);

    JobBuilder withUpstreams(UpstreamType type, JobBuilder... upstreams);

    JobBuilder withUpstreams(UpstreamType type, UpstreamStyle style, JobBuilder... upstreams);

    /**
     * Определяет соедининия ({@link Job}), от которых зависит текущая джоба.
     *
     * @param upstreams список апстримов
     * @return <T> - a JobBuilder
     */
    JobBuilder withUpstreams(Collection<? extends JobBuilder> upstreams);

    JobBuilder withUpstreams(UpstreamType type, Collection<? extends JobBuilder> upstreams);

    JobBuilder withUpstreams(UpstreamType type, UpstreamStyle style, Collection<? extends JobBuilder> upstreams);

    JobBuilder withUpstreams(Multimap<UpstreamType, ? extends JobBuilder> upstreams);

    JobBuilder withUpstreams(CanRunWhen canRunWhen, JobBuilder... upstreams);

    JobBuilder withUpstreams(CanRunWhen canRunWhen, UpstreamType type, JobBuilder... upstreams);

    JobBuilder withUpstreams(CanRunWhen canRunWhen, List<? extends JobBuilder> upstreams);

    JobBuilder withUpstreams(CanRunWhen canRunWhen, UpstreamType type, List<? extends JobBuilder> upstreams);

    JobBuilder withRetry(JobAttemptsConfig retry);

    JobBuilder withCanRunWhen(CanRunWhen canRunWhen);

    /**
     * Иногда с джобой нужно передать какую-то дополнительную информацию,
     * Например, в мультитестингах, предполагается передавать:
     * 1. Является ли эта джоба закрывающей флоу,
     * 2. Является ли эта джоба завершающей очистку среды.
     *
     * @param tags список тегов
     * @return <T> - a JobBuilder
     */
    JobBuilder withTags(String... tags);

    /**
     * Иногда с джобой нужно передать какую-то дополнительную информацию,
     * Например, в мультитестингах, предполагается передавать:
     * 1. Является ли эта джоба закрывающей флоу,
     * 2. Является ли эта джоба завершающей очистку среды.
     *
     * @param tags список тегов
     * @return <T> - a JobBuilder
     */
    JobBuilder withTags(List<String> tags);

    /**
     * Начиная с данной джобы, начнётся новая синхронизированная стадия.
     *
     * @param stage стадия
     * @return <T> - a JobBuilder
     */
    JobBuilder beginStage(Stage stage);

    /**
     * Если с данной джобы начинается новая стадия, можно сделать так, чтобы при взятии новой стадии, прошлая стадия
     * не разлочивалась до тех пор, пока все джобы с holdPreviousStage не завершатся успехом.
     * <p>
     * На практике это может быть нужно, когда хочется чтобы флоу не позволял следующему флоу катиться
     * в тестинг, пока он сам не выложится в престейбл.
     *
     * @return <T> - a JobBuilder
     */
    JobBuilder holdPreviousStage();

    JobBuilder withPosition(Position position);

    JobBuilder withInternal(InternalExecutorContext internal);

    JobBuilder withTasklet(TaskletExecutorContext tasklet);

    JobBuilder withTaskletV2(TaskletV2ExecutorContext taskletV2);

    JobBuilder withSandboxTask(SandboxExecutorContext sandboxTask);

    JobBuilder withProperties(JobProperties properties);

    JobBuilder withSkippedByMode(JobSkippedByMode skippedByMode);

    /**
     * Указывает на то, что джоба должна триггериться только вручную.
     * Отличается от {@link JobBuilder#withPrompt()} тем,
     * что работает без дополнительного подтверждения в виде диалогового окна "Вы уверены?".
     * Без этой опции, джоба триггерится автоматически по завершению всех её апстримов.
     *
     * @return JobBuilder
     */
    JobBuilder withManualTrigger();

    /**
     * Аналогично {@link JobBuilder#withManualTrigger()}, только с подтверждением — диалоговым окном.
     * Это полезно для критичных джоб, вроде выкладки в продакшн, где необходимо подтверждение человека.
     *
     * @param question Текст подтверждения
     * @return JobBuilder
     */
    JobBuilder withPrompt(String question);

    /**
     * Аналогично {@link JobBuilder#withManualTrigger()}, только с подтверждением — диалоговым окном.
     * Это полезно для критичных джоб, вроде выкладки в продакшн, где необходимо подтверждение человека.
     *
     * @return JobBuilder
     */
    JobBuilder withPrompt();

    /***
     * Указывает на то, что джоба должна быть выполнена только если выполнено определенное условие (задаваемое
     * выражением JMESPath). Работает вместе с любыми другими условиями.
     *
     * @param expression JMESPath выражение
     * @return JobBuilder
     */
    JobBuilder withConditionalRunExpression(String expression);

    JobBuilder withRequirements(RequirementsConfig requirements);

    JobBuilder withJobRuntimeConfig(RuntimeConfig jobRuntimeConfig);

    JobSchedulerBuilder withScheduler();

    JobBuilder withSchedulerConstraint(JobSchedulerConstraintEntity schedulerConstraint);

    //

    String getId();

    String getTitle();

    String getDescription();

    JobMultiplyConfig getMultiply();

    boolean hasManualTrigger();

    ManualTriggerPrompt getManualTriggerPrompt();

    InternalExecutorContext getInternal();

    TaskletExecutorContext getTasklet();

    TaskletV2ExecutorContext getTaskletV2();

    SandboxExecutorContext getSandboxTask();

    List<Resource> getResources();

    Set<UpstreamLink<JobBuilder>> getUpstreams();

    Set<String> getTags();

    CanRunWhen getCanRunWhen();

    String getConditionalRunExpression();

    JobAttemptsConfig getRetry();

    /**
     * Возвращает стадию синхронизации, которой принадлежит данная джоба.
     *
     * @return Stage
     */
    Stage getStage();

    boolean getShouldHoldPreviousStage();

    Position getPosition();

    RequirementsConfig getRequirements();

    RuntimeConfig getJobRuntimeConfig();

    JobType getJobType();

    JobProperties getProperties();

    JobSchedulerConstraintEntity getJobSchedulerConstraint();

    JobSkippedByMode getSkippedByMode();

    //

    interface JobSchedulerBuilder {

        JobSchedulerBuilder workDaysHours(int hoursFrom, int hoursTo);

        JobSchedulerBuilder workDaysHours(DayOfWeek dayOfWeek, int hoursFrom, int hoursTo);

        JobSchedulerBuilder workDays(int hoursFrom, int minutesFrom, int hoursTo, int minutesTo);

        JobSchedulerBuilder workDays(DayOfWeek dayOfWeek, int hoursFrom, int minutesFrom, int hoursTo, int minutesTo);

        JobSchedulerBuilder preHolidayHours(int hoursFrom, int hoursTo);

        JobSchedulerBuilder preHolidayHours(DayOfWeek dayOfWeek, int hoursFrom, int hoursTo);

        JobSchedulerBuilder preHoliday(int hoursFrom, int minutesFrom, int hoursTo, int minutesTo);

        JobSchedulerBuilder preHoliday(DayOfWeek dayOfWeek, int hoursFrom, int minutesFrom, int hoursTo, int minutesTo);

        JobSchedulerBuilder holidayHours(int hoursFrom, int hoursTo);

        JobSchedulerBuilder holidayHours(DayOfWeek dayOfWeek, int hoursFrom, int hoursTo);

        JobSchedulerBuilder holiday(int hoursFrom, int minutesFrom, int hoursTo, int minutesTo);

        JobSchedulerBuilder holiday(DayOfWeek dayOfWeek, int hoursFrom, int minutesFrom, int hoursTo, int minutesTo);

        JobBuilder build();
    }
}
