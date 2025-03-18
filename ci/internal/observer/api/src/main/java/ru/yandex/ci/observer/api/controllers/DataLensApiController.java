package ru.yandex.ci.observer.api.controllers;

import java.time.Instant;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.ci.observer.api.statistics.ChecksCountService;
import ru.yandex.ci.observer.api.statistics.DetailedStatisticsService;
import ru.yandex.ci.observer.api.statistics.aggregated.AggregatedStatisticsService;
import ru.yandex.ci.observer.api.statistics.model.IterationWithArcanumInfo;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check.ChecksCountStatement;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.sla_statistics.IterationCompleteGroup;
import ru.yandex.ci.observer.core.db.model.sla_statistics.IterationTypeGroup;
import ru.yandex.ci.observer.core.db.model.sla_statistics.SlaStatisticsEntity;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

@RestController
@RequestMapping(
        method = RequestMethod.GET,
        path = "/api/datalens",
        produces = MediaType.APPLICATION_JSON_VALUE
)
public class DataLensApiController {

    @Nonnull
    private final DetailedStatisticsService detailedStats;
    @Nonnull
    private final AggregatedStatisticsService aggregatedStats;
    @Nonnull
    private final ChecksCountService checksCountService;

    @Autowired
    public DataLensApiController(
            DetailedStatisticsService detailedStats,
            AggregatedStatisticsService aggregatedStats,
            ChecksCountService checksCountService
    ) {
        this.detailedStats = detailedStats;
        this.aggregatedStats = aggregatedStats;
        this.checksCountService = checksCountService;
    }

    @GetMapping(path = "/detailed/iterations")
    public ResponseEntity<List<CheckIterationEntity>> getDetailedIterations(
            @RequestParam(value = "from") Instant from,
            @RequestParam(value = "to") Instant to,
            @RequestParam(value = "checkTypes") CheckOuterClass.CheckType checkType,
            @RequestParam(value = "iterationTypes", required = false, defaultValue = "")
            List<CheckIteration.IterationType> iterationTypes,
            @RequestParam(value = "advisedPool", required = false) String advisedPool,
            @RequestParam(value = "hidePessimized", required = false, defaultValue = "true") boolean hidePessimized,
            @RequestParam(value = "hideFinished", required = false, defaultValue = "false") boolean hideFinished,
            @RequestParam(value = "hideCancelled", required = false, defaultValue = "false") boolean hideCancelled,
            @RequestParam(value = "authors", required = false, defaultValue = "") List<String> authors
    ) {
        return ResponseEntity.ok(detailedStats.getDetailedAutocheckStagesByIterations(
                from, to, checkType, iterationTypes, advisedPool, hidePessimized, hideFinished, hideCancelled, authors
        ));
    }

    @GetMapping(path = "/detailed/iteration")
    public ResponseEntity<IterationWithArcanumInfo> getIteration(
            @RequestParam(value = "checkId") Long checkId,
            @RequestParam(value = "iterationType") CheckIteration.IterationType iterationType,
            @RequestParam(value = "iterationNumber") Integer iterationNumber
    ) {
        CheckIterationEntity.Id iterationId = new CheckIterationEntity.Id(
                CheckEntity.Id.of(checkId), iterationType, iterationNumber
        );

        var iteration = detailedStats.getIteration(iterationId);
        if (iteration.isEmpty()) {
            return ResponseEntity.notFound().build();
        }

        return ResponseEntity.ok(iteration.get());
    }

    @GetMapping(path = "/detailed/tasks")
    public ResponseEntity<List<CheckTaskEntity>> getIterationTasks(
            @RequestParam(value = "checkId") Long checkId,
            @RequestParam(value = "iterationType") CheckIteration.IterationType iterationType,
            @RequestParam(value = "iterationNumber") Integer iterationNumber
    ) {
        CheckIterationEntity.Id iterationId = new CheckIterationEntity.Id(
                CheckEntity.Id.of(checkId), iterationType, iterationNumber
        );

        return ResponseEntity.ok(detailedStats.getIterationTasksWithRecheck(iterationId));
    }

    @GetMapping(path = "/aggregated/sla")
    public ResponseEntity<List<SlaStatisticsEntity>> getSlaStatistics(
            @RequestParam(value = "from") Instant from,
            @RequestParam(value = "to") Instant to,
            @RequestParam(value = "checkType") CheckOuterClass.CheckType checkType,
            @RequestParam(value = "iterationTypeGroup") IterationTypeGroup iterationTypeGroup,
            @RequestParam(value = "status") IterationCompleteGroup status,
            @RequestParam(value = "windowDays") int windowDays,
            @RequestParam(value = "advisedPool", required = false) String advisedPool,
            @RequestParam(value = "totalNumberOfNodes", required = false) Long totalNumberOfNodes,
            @RequestParam(value = "authors", required = false, defaultValue = "") Set<String> authors,
            @RequestParam(value = "recalculateSelected", required = false, defaultValue = "false")
            boolean recalculateSelected
    ) {
        return ResponseEntity.ok(aggregatedStats.getSlaStatistics(
                from, to, checkType, iterationTypeGroup, status, windowDays,
                advisedPool, authors, totalNumberOfNodes, recalculateSelected
        ));
    }

    @GetMapping(path = "/checks/count")
    public ResponseEntity<List<ChecksCountStatement.CountByInterval>> getChecksCount(
            @RequestParam(value = "from") Instant from,
            @RequestParam(value = "to", required = false) Instant to,
            @RequestParam(value = "interval") IntervalDto interval,
            @RequestParam(value = "includeIncompleteInterval") boolean includeIncompleteInterval,
            @RequestParam(value = "authorFilter", defaultValue = "all") AuthorFilterDto authorFilter
    ) {
        return ResponseEntity.ok(
                checksCountService.count(from, to, interval, includeIncompleteInterval, authorFilter)
        );
    }
}
