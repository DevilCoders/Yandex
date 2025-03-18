package ru.yandex.ci.observer.api.controllers;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.time.DurationParser;
import ru.yandex.ci.observer.api.stress_test.CheckRevisionsDto;
import ru.yandex.ci.observer.api.stress_test.GetUsedRevisionsRequestDto;
import ru.yandex.ci.observer.api.stress_test.OrderedRevisionDto;
import ru.yandex.ci.observer.api.stress_test.StressTestService;
import ru.yandex.ci.observer.api.stress_test.UsedRevisionResponseDto;
import ru.yandex.ci.observer.core.db.model.stress_test.StressTestUsedCommitEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

@RestController
@RequestMapping(
        method = RequestMethod.GET,
        path = "/api/v1/stress-test",
        produces = MediaType.APPLICATION_JSON_VALUE
)
public class StressTestController {

    private final StressTestService stressTestService;

    @Autowired
    public StressTestController(StressTestService stressTestService) {
        this.stressTestService = stressTestService;
    }

    @GetMapping(path = "not-used-revisions")
    public ResponseEntity<List<CheckRevisionsDto>> getNotUsedRevisions(
            @RequestParam(value = "startRevision") String startRevision,
            @RequestParam(value = "duration") String duration,
            @RequestParam(value = "revisionPerHour") int revisionsPerHour,
            @RequestParam(value = "namespace") String namespace
    ) {
        var revisions = stressTestService.getNotUsedRevisions(
                ArcRevision.parse(startRevision),
                DurationParser.parse(duration),
                revisionsPerHour,
                namespace
        );

        return ResponseEntity.ok(
                revisions.stream()
                        .map(it -> CheckRevisionsDto.of(
                                toOrderedRevisionDto(it.getLeft()),
                                toOrderedRevisionDto(it.getRight()),
                                it.getDiffSetId()
                        ))
                        .toList()
        );
    }

    @PostMapping(path = "mark-as-used")
    public ResponseEntity<Void> markAsUsed(
            @RequestParam(value = "rightRevision") String rightRevision,
            @RequestParam(value = "leftRevision") String leftRevision,
            @RequestParam(value = "namespace") String namespace,
            @RequestParam(value = "flowLaunchId") String flowLaunchId
    ) {
        stressTestService.markAsUsed(
                ArcRevision.parse(rightRevision),
                ArcRevision.parse(leftRevision),
                namespace,
                flowLaunchId
        );

        return ResponseEntity.ok().build();
    }

    @PostMapping(
            path = "find-used-revisions",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public ResponseEntity<List<UsedRevisionResponseDto>> findUsedRevisions(
            @RequestBody GetUsedRevisionsRequestDto request
    ) {
        var usedRevisions = stressTestService.findUsedRightRevisions(request.getRightRevisions(),
                request.getNamespace());

        return ResponseEntity.ok(
                usedRevisions.stream()
                        .map(StressTestController::toUsedRevisionResponseDto)
                        .toList()
        );
    }

    @ExceptionHandler(IllegalArgumentException.class)
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public ResponseEntity<Object> handle(IllegalArgumentException e) {
        return ResponseEntity.badRequest().body(e.getMessage());
    }

    @ExceptionHandler(Exception.class)
    @ResponseStatus(HttpStatus.INTERNAL_SERVER_ERROR)
    public ResponseEntity<Object> handle(Exception e) {
        return ResponseEntity.internalServerError().body(e.getMessage());
    }

    private static OrderedRevisionDto toOrderedRevisionDto(StorageRevision revision) {
        return OrderedRevisionDto.of(
                revision.getBranch(),
                revision.getRevision(),
                revision.getRevisionNumber()
        );
    }

    private static UsedRevisionResponseDto toUsedRevisionResponseDto(StressTestUsedCommitEntity source) {
        return new UsedRevisionResponseDto(
                source.getId().getRightCommitId(),
                source.getId().getNamespace(),
                source.getId().getLeftRevisionNumber(),
                source.getFlowLaunchId()
        );
    }

}
