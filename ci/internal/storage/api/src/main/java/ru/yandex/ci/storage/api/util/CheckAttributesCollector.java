package ru.yandex.ci.storage.api.util;

import java.util.ArrayList;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import org.apache.logging.log4j.util.Strings;

import ru.yandex.ci.common.info.InfoPanelOuterClass;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.flow.CiActionReference;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.core.proto.InfoPanelUtils;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.TestTypeStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.utils.YandexUrls;

@RequiredArgsConstructor
public class CheckAttributesCollector {
    @Nonnull
    private final FlowUrls flowUrls;

    public InfoPanelOuterClass.InfoPanel fillCheckAttributes(CheckEntity check) {
        InfoPanelOuterClass.InfoEntity reviewAttribute;
        if (check.getType().equals(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT) ||
                check.getType().equals(CheckOuterClass.CheckType.BRANCH_PRE_COMMIT)) {
            var reviewId = ArcBranch.ofString(check.getRight().getBranch()).getPullRequestId();
            reviewAttribute = InfoPanelUtils.createAttribute(
                    "Review",
                    Long.toString(reviewId),
                    YandexUrls.reviewUrl(reviewId),
                    true
            );
        } else {
            reviewAttribute = InfoPanelUtils.createAttribute(
                    "Review", "-", false
            );
        }

        return InfoPanelOuterClass.InfoPanel.newBuilder()
                .addEntities(reviewAttribute)
                .addEntities(InfoPanelUtils.createAttribute("Left branch", check.getLeft().getBranch(), true))
                .addEntities(InfoPanelUtils.createAttribute("Right branch", check.getRight().getBranch(), true))
                .addEntities(createRevisionAttribute("Left revision", check.getLeft()))
                .addEntities(createRevisionAttribute("Right revision", check.getRight()))
                .build();
    }

    public InfoPanelOuterClass.InfoPanel fillIterationAttributes(
            CheckEntity check,
            CheckIterationEntity iteration
    ) {
        var attributes = new IterationAttributesCollector(check, iteration, iteration.getId()).fillAttributes();
        return InfoPanelOuterClass.InfoPanel.newBuilder()
                .addAllEntities(attributes)
                .build();
    }

    //

    private InfoPanelOuterClass.InfoEntity createRevisionAttribute(String name, StorageRevision revision) {
        String commitId = revision.getArcanumCommitId();

        return InfoPanelUtils.createAttribute(
                name,
                commitId,
                YandexUrls.revisionUrl(commitId),
                true
        );
    }

    @RequiredArgsConstructor
    private class IterationAttributesCollector {
        private final ArrayList<InfoPanelOuterClass.InfoEntity> attributes = new ArrayList<>();
        @Nonnull
        private final CheckEntity check;
        @Nonnull
        private final CheckIterationEntity iteration;
        @Nonnull
        private final CheckIterationEntity.Id iterationId;

        public ArrayList<InfoPanelOuterClass.InfoEntity> fillAttributes() {
            attributes.add(InfoPanelUtils.createAttribute(
                            "Iteration id",
                            iterationId.toPureString(),
                            YandexUrls.ciBadgeUrl(iterationId.getCheckId()),
                            true
                    )
            );
            if (Strings.isNotEmpty(iteration.getInfo().getAdvisedPool())) {
                attributes.add(
                        InfoPanelUtils.createAttribute(
                                "Advised pool",
                                iteration.getInfo().getAdvisedPool(),
                                YandexUrls.distPoolUrl(iteration.getInfo().getAdvisedPool()),
                                true
                        )
                );
            }

            attributes.add(
                    InfoPanelUtils.createAttribute(
                            "Strong mode policy", iteration.getInfo().getStrongModePolicy().name(), true
                    )
            );

            if (iterationId.getIterationType() == CheckIteration.IterationType.FAST
                    || iterationId.getIterationType() == CheckIteration.IterationType.FULL) {
                attributes.add(
                        InfoPanelUtils.createAttribute(
                                "Execution profiler",
                                "datalens",
                                YandexUrls.ciExecutionProfiler(iterationId),
                                true
                        )
                );
            }

            var reviewId = check.getType().equals(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT) ||
                    check.getType().equals(CheckOuterClass.CheckType.BRANCH_PRE_COMMIT) ?
                    ArcBranch.ofString(check.getRight().getBranch()).getPullRequestId() : null;

            var metrics = iteration.getStatistics().getMetrics();

            if (!iteration.getId().isHeavy()) {
                var technical = iteration.getStatistics().getTechnical();
                var numberOfNodes = metrics.getValueOrElse(Metrics.Name.NUMBER_OF_NODES,
                        technical.getTotalNumberOfNodes());
                attributes.add(
                        InfoPanelUtils.createAttribute("Number of nodes", numberOfNodes, false)
                );

                if (reviewId != null) {
                    attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "Consumed CPU",
                                    InfoPanelUtils.formatSeconds(metrics.getValueOrZero("slot_time")),
                                    YandexUrls.distbuildProfilerUrl(reviewId),
                                    false
                            )
                    );
                } else {
                    attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "Consumed CPU",
                                    InfoPanelUtils.formatSeconds(metrics.getValueOrZero("slot_time")),
                                    false
                            )
                    );
                }
            }

            attributes.add(
                    InfoPanelUtils.createAttribute(
                            "Completed tasks",
                            "%d / %d".formatted(
                                    iteration.getNumberOfCompletedTasks(),
                                    Math.max(iteration.getNumberOfTasks(), iteration.getExpectedTasks().size())
                            ),
                            YandexUrls.autocheckSandboxTasksUrl(iterationId),
                            false
                    )
            );
            attributes.add(
                    InfoPanelUtils.createAttribute(
                            "Required tasks registered",
                            "%d / %d".formatted(
                                    iteration.getRegisteredExpectedTasks().size(), iteration.getExpectedTasks().size()
                            ),
                            false
                    )
            );


            Optional.ofNullable(getLargeAutostartStatus())
                    .ifPresent(status -> attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "Large Tests auto-start",
                                    status,
                                    false
                            )
                    ));

            Optional.ofNullable(getNativeBuildsStatus())
                    .ifPresent(status -> attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "Native Builds auto-start",
                                    status,
                                    false
                            )
                    ));


            addTestTypeStatisticsAttributes();
            addFlowLinks();

            return attributes;
        }

        private void addFlowLinks() {
            var ciActionReferences = iteration.getInfo().getCiActionReferences();
            for (var i = 1; i <= ciActionReferences.size(); i++) {
                CiActionReference ref = ciActionReferences.get(i - 1);
                attributes.add(
                        InfoPanelUtils.createAttribute(
                                "Flow %s".formatted(i > 1 ? i : ""),
                                "link",
                                flowUrls.toFlowLaunch(
                                        "autocheck",
                                        ref.getFlowId().getDir(),
                                        ref.getFlowId().getId(),
                                        ref.getLaunchNumber()
                                ),
                                false
                        )
                );
            }
        }


        private void addTestTypeStatisticsAttributes() {
            var testTypeStatistics = iteration.getTestTypeStatistics();
            if (iteration.isHeavy()) {
                if (testTypeStatistics.getLargeTests().hasStarted()) {
                    var large = testTypeStatistics.getLargeTests();
                    attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "Large",
                                    "%d / %d".formatted(large.getCompletedTasks(), large.getRegisteredTasks()),
                                    YandexUrls.largeSandboxTasksUrl(iteration.getId()),
                                    false
                            )
                    );
                }

                if (testTypeStatistics.getNativeBuilds().hasStarted()) {
                    var nativeBuilds = testTypeStatistics.getNativeBuilds();
                    attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "Native builds",
                                    "%d / %d".formatted(
                                            nativeBuilds.getCompletedTasks(), nativeBuilds.getRegisteredTasks()
                                    ),
                                    YandexUrls.nativeSandboxTasksUrl(iteration.getId()),
                                    false
                            )
                    );
                }

                if (testTypeStatistics.getTeTests().hasStarted()) {
                    var teId = Strings.isNotEmpty(check.getTestenvId()) ?
                            check.getTestenvId() :
                            iteration.getTestenvId() == null ? "" : iteration.getTestenvId();

                    var te = testTypeStatistics.getTeTests();
                    attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "TestEnv",
                                    "%d / %d".formatted(te.getCompletedTasks(), te.getRegisteredTasks()),
                                    YandexUrls.testenvJobsSandboxTasksUrl(teId),
                                    false
                            )
                    );

                    attributes.add(
                            InfoPanelUtils.createAttribute(
                                    "TestEnv",
                                    "timeline",
                                    YandexUrls.testenvTimelineUrl(teId),
                                    false
                            )
                    );
                }
            } else {
                addProgressAttribute("Configure", testTypeStatistics.getConfigure(), "");
                addProgressAttribute("Build", testTypeStatistics.getBuild(), "");
                addProgressAttribute("Style", testTypeStatistics.getStyle(), "");
                addProgressAttribute("Small", testTypeStatistics.getSmallTests(), "");
                addProgressAttribute("Medium", testTypeStatistics.getMediumTests(), "");
            }
        }

        private void addProgressAttribute(
                String type,
                TestTypeStatistics.Statistics statistics,
                String url
        ) {
            attributes.add(
                    InfoPanelUtils.createAttribute(
                            type + " progress",
                            getProgress(statistics),
                            url,
                            false
                    )
            );
        }

        private String getProgress(TestTypeStatistics.Statistics statistics) {
            if (statistics.isCompleted()) {
                return "done";
            }

            if (statistics.isWaitingForConfigure() || statistics.isWaitingForChunks()) {
                return "completing";
            }

            return Math.round(statistics.getCompletedPercent()) + "%";
        }

        @Nullable
        private String getLargeAutostartStatus() {
            return iteration.getId().isFirstFullIteration()
                    ? getCheckTaskStatus(iteration, Common.CheckTaskType.CTT_LARGE_TEST)
                    : null;
        }

        @Nullable
        private String getNativeBuildsStatus() {
            return iteration.getId().isFirstFullIteration()
                    ? getCheckTaskStatus(iteration, Common.CheckTaskType.CTT_NATIVE_BUILD)
                    : null;
        }

        private String getCheckTaskStatus(CheckIterationEntity iteration,
                                          Common.CheckTaskType checkTaskType) {
            var status = iteration.getCheckTaskStatus(checkTaskType);
            return switch (status) {
                case NOT_REQUIRED -> "Not required";
                case MAYBE_DISCOVERING -> "Discovering";
                case DISCOVERING -> "Discovering for execution";
                case SCHEDULED -> "Scheduled for execution";
                case COMPLETE -> "Complete";
            };
        }
    }
}
