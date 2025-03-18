package ru.yandex.ci.storage.core.check;

import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.constant.StorageLimits;
import ru.yandex.ci.storage.core.db.constant.TestenvUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;
import ru.yandex.ci.storage.core.exceptions.CheckIsReadonlyException;
import ru.yandex.ci.storage.core.exceptions.IterationInWrongStatusException;
import ru.yandex.ci.storage.core.exceptions.TaskAlreadyRegisteredException;
import ru.yandex.ci.util.OurGuys;

@Slf4j
@AllArgsConstructor
public class CheckService {

    protected final RequirementsService requirementsService;

    public List<CheckIterationEntity> getActiveIterations(
            IterationsCache iterationsCache, CheckEntity.Id checkId
    ) {
        return getActiveIterations(iterationsCache.getFreshForCheck(checkId));
    }

    protected List<CheckIterationEntity> getActiveIterations(Collection<CheckIterationEntity> iterations) {
        return iterations.stream()
                .filter(x -> CheckStatusUtils.isActive(x.getStatus()))
                .collect(Collectors.toList());
    }

    public CheckIterationEntity registerIterationInTx(
            StorageCoreCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            CreateIterationParams params
    ) {
        var check = cache.checks().getFreshOrThrow(iterationId.getCheckId());

        var attributes = new HashMap<Common.StorageAttribute, String>();
        if (OurGuys.isOurGuy(check.getAuthor())) {
            attributes.put(Common.StorageAttribute.SA_IS_OWNER_OUR_GUY, "yes");
        }

        params = params.toBuilder().attributes(attributes).build();

        if (iterationId.getNumber() > 0) {
            var existingOptional = cache.iterations().getFresh(iterationId);

            if (existingOptional.isPresent()) {
                var existing = existingOptional.get();
                log.info("Iteration already registered {}", existing.getId());
                return existing;
            }
        } else {
            var currentIterations = cache.iterations().getFreshForCheck(iterationId.getCheckId());
            var last = getLastIteration(currentIterations, iterationId.getIterationType());
            iterationId = iterationId.toIterationId(last.isEmpty() ? 1 : last.get().getId().getNumber() + 1);

            log.info(
                    "Assigned id {} for new iteration, current iterations: {}",
                    iterationId, currentIterations
            );

            // Mark iteration in transaction for consistency
            Preconditions.checkState(cache.iterations().getFresh(iterationId).isEmpty());
        }

        if (check.isReadOnly()) {
            throw new CheckIsReadonlyException(check.getId(), "Can't register iteration: " + iterationId);
        }

        // todo CI-2773 temporary disabled
        if (check.getType().equals(CheckOuterClass.CheckType.TRUNK_POST_COMMIT)) {
            params = params.toBuilder()
                    .expectedTasks(
                            params.getExpectedTasks().stream().filter(ExpectedTask::isRight).collect(Collectors.toSet())
                    )
                    .build();
        }

        if (params.getTasksType().equals(Common.CheckTaskType.CTT_TESTENV)) {
            params = params.toBuilder()
                    .expectedTasks(
                            Set.of(
                                    new ExpectedTask(
                                            TestenvUtils.ALLOW_FINISH_TASK_PREFIX + iterationId.getNumber(), false
                                    )
                            )
                    )
                    .build();

            requirementsService.scheduleRequirement(
                    cache, check.getId(), ArcanumCheckType.TE_JOBS, ArcanumCheckStatus.pending()
            );
        }

        log.info(
                "Registering iteration {}, params: {}", iterationId, params
        );

        params = params.toBuilder()
                .chunkShift(iterationId.generateChunkShift())
                .build();

        var iteration = CheckIterationEntity.create(iterationId, params);
        iteration = withHeavyTasks(cache, check, iteration); // todo migrate this to params

        if (CheckStatusUtils.isCompleted(check.getStatus())) {
            cache.checks().writeThrough(check.withStatus(Common.CheckStatus.RUNNING));
        }

        if (params.isAutorun()) {
            iteration = iteration.withStatus(Common.CheckStatus.RUNNING);
        }

        cache.iterations().writeThrough(iteration);

        if (iterationId.getNumber() > 1) {
            registerMetaIteration(cache, iteration);
        }

        return iteration;
    }

    public Optional<CheckIterationEntity> getLastIteration(
            Collection<CheckIterationEntity> iterations, CheckIteration.IterationType iterationType
    ) {
        return iterations
                .stream().filter(x -> x.getId().getIterationType().equals(iterationType))
                .max(Comparator.comparing(x -> x.getId().getNumber()));
    }

    private void registerMetaIteration(
            StorageCoreCache.Modifiable cache,
            CheckIterationEntity iteration
    ) {
        var metaIterationId = CheckIterationEntity.Id.of(iteration.getId().getCheckId(),
                iteration.getId().getIterationType(), 0);
        var existingOptional = cache.iterations().getFresh(metaIterationId);

        CheckIterationEntity metaIteration;
        if (existingOptional.isPresent()) {
            log.info("Restarting meta iteration {}", existingOptional.get());
            metaIteration = IterationUtils.restartMetaIteration(iteration.getExpectedTasks(), existingOptional.get());
        } else {
            var firstIteration = cache.iterations().getFresh(iteration.getId().toIterationId(1))
                    .orElse(iteration);

            log.info("Registering meta iteration {} from {}", metaIterationId, firstIteration.getId());
            metaIteration = IterationUtils.createMetaIteration(firstIteration, iteration.getExpectedTasks());

            if (iteration.getStatus() == Common.CheckStatus.RUNNING) {
                metaIteration = metaIteration.withStatus(Common.CheckStatus.RUNNING);
            }
        }

        cache.iterations().writeThrough(metaIteration);
    }

    private CheckIterationEntity withHeavyTasks(
            StorageCoreCache.Modifiable cache,
            CheckEntity check,
            CheckIterationEntity iteration
    ) {
        if (!iteration.getId().isFirstFullIteration()
                || requirementsService.skipCheck(check, "Heavy tasks")) {
            return iteration;
        }

        var largeTestsStatus = check.getAutostartLargeTests().isEmpty()
                ? CheckTaskStatus.MAYBE_DISCOVERING
                : CheckTaskStatus.DISCOVERING;

        var nativeBuildsStatus = check.getNativeBuilds().isEmpty()
                ? CheckTaskStatus.NOT_REQUIRED
                : CheckTaskStatus.DISCOVERING;

        var skipLargeAutostart = largeTestsStatus == CheckTaskStatus.MAYBE_DISCOVERING;
        var skipNativeAutostart = nativeBuildsStatus == CheckTaskStatus.NOT_REQUIRED;

        requirementsService.scheduleRequirement(
                cache, check.getId(), ArcanumCheckType.CI_LARGE_TESTS,
                autostartConfiguredToArcanumCheckStatus(skipLargeAutostart)
        );

        requirementsService.scheduleRequirement(
                cache, check.getId(), ArcanumCheckType.CI_BUILD_NATIVE,
                autostartConfiguredToArcanumCheckStatus(skipNativeAutostart)
        );

        return iteration
                .updateCheckTaskStatuses(Map.of(
                        Common.CheckTaskType.CTT_LARGE_TEST, largeTestsStatus,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, nativeBuildsStatus
                ));
    }

    private ArcanumCheckStatus autostartConfiguredToArcanumCheckStatus(boolean skipLargeAutostart) {
        return skipLargeAutostart ?
                ArcanumCheckStatus.skipped("No Autostart configured") :
                ArcanumCheckStatus.pending();
    }

    public CheckTaskEntity registerTaskInTx(StorageCoreCache.Modifiable cache, CheckTaskEntity task) {
        var existingOptional = cache.checkTasks().getFresh(task.getId());
        if (existingOptional.isPresent()) {
            var existing = existingOptional.get();
            if (
                    existing.getJobName().equals(task.getJobName()) &&
                            existing.getNumberOfPartitions() == task.getNumberOfPartitions() &&
                            existing.isRight() == task.isRight()
            ) {
                return existing;
            } else {
                throw new TaskAlreadyRegisteredException(task.getId());
            }
        }

        var metaIterationOptional = cache.iterations().getFresh(task.getId().getIterationId().toMetaId());

        return registerNotExistingTaskInTx(
                cache, task, task.getId().getIterationId(),
                metaIterationOptional.map(CheckIterationEntity::getId).orElse(null)
        );
    }

    public CheckTaskEntity registerNotExistingTaskInTx(
            StorageCoreCache.Modifiable cache,
            CheckTaskEntity task,
            CheckIterationEntity.Id iterationId,
            @Nullable CheckIterationEntity.Id metaIterationId
    ) {
        var metaIteration = metaIterationId == null ? null : cache.iterations().getFreshOrThrow(metaIterationId);
        var iteration = cache.iterations().getFreshOrThrow(iterationId);

        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            throw new IterationInWrongStatusException(
                    "Iteration %s is not running, status: %s".formatted(iteration.getId(), iteration.getStatus())
            );
        }

        log.info(
                "Registering task, iteration: {}, tasks: {}, tasks in meta: {}, task id: {}",
                iteration.getId(),
                iteration.getNumberOfTasks(),
                metaIteration == null ? "-" : metaIteration.getNumberOfTasks(),
                task.getId()
        );

        if (iteration.getNumberOfTasks() >= StorageLimits.TASKS_LIMIT) {
            throw GrpcUtils.resourceExhausted(
                    "Iteration %s already has %d tasks".formatted(iteration.getId(), iteration.getNumberOfTasks())
            );
        }

        var testTypeStatistics = iteration.getTestTypeStatistics().onRegistered(task.getType());

        var info = iteration.getInfo().toBuilder()
                .progress(testTypeStatistics.calculateProgress())
                .build();

        iteration = iteration.toBuilder()
                .numberOfTasks(iteration.getNumberOfTasks() + 1)
                .registeredExpectedTasks(updateRegisteredExpectedTasks(task, iteration))
                .testTypeStatistics(testTypeStatistics)
                .info(info)
                .build();

        cache.iterations().writeThrough(iteration);

        if (metaIteration != null) {
            var metaTestTypeStatistics = metaIteration.getTestTypeStatistics().onRegistered(task.getType());

            var metaInfo = metaIteration.getInfo().toBuilder()
                    .progress(metaTestTypeStatistics.calculateProgress())
                    .build();

            metaIteration = metaIteration.toBuilder()
                    .numberOfTasks(metaIteration.getNumberOfTasks() + 1)
                    .registeredExpectedTasks(updateRegisteredExpectedTasks(task, metaIteration))
                    .info(metaInfo)
                    .testTypeStatistics(metaTestTypeStatistics)
                    .build();

            cache.iterations().writeThrough(metaIteration);
        }

        cache.checkTasks().writeThrough(task);

        log.info(
                "Task registered {}, iteration number of tasks: {}, tasks in meta: {}, expected tasks: {}/{}",
                task.getId(),
                iteration.getNumberOfTasks(),
                metaIteration == null ? "-" : metaIteration.getNumberOfTasks(),
                iteration.getRegisteredExpectedTasks().size(),
                iteration.getExpectedTasks().size()
        );

        return task;
    }

    private Set<ExpectedTask> updateRegisteredExpectedTasks(CheckTaskEntity task, CheckIterationEntity iteration) {
        var registeredExpectedTasks = iteration.getRegisteredExpectedTasks();

        var expect = new ExpectedTask(task.getJobName(), task.isRight());
        if (iteration.getExpectedTasks().contains(expect)) {
            registeredExpectedTasks = new HashSet<>(registeredExpectedTasks);
            registeredExpectedTasks.add(expect);
        }

        return registeredExpectedTasks;
    }
}
