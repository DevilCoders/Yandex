package ru.yandex.ci.engine.launch.auto;

import java.nio.file.Path;
import java.util.List;
import java.util.NoSuchElementException;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.junit.jupiter.params.provider.ValueSource;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.client.storage.StorageApiTestServer;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.commune.bazinga.impl.TaskId;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.core.launch.PostponeLaunch.StartReason.BINARY_SEARCH;
import static ru.yandex.ci.core.launch.PostponeLaunch.StartReason.DEFAULT;
import static ru.yandex.ci.core.launch.PostponeLaunch.StopReason.OBSOLETE_LAUNCH;

@SuppressWarnings("MethodLength")
class LargePostCommitsTest extends AbstractLargePostCommitsTest {

    @Autowired
    protected LargePostCommitHandler handler;

    @Autowired
    protected StorageApiTestServer storageApiTestServer;

    @BeforeEach
    @Override
    void beforeEach() {
        super.beforeEach();
        storageApiTestServer.clear();
        compareLargeTestsAll();
    }

    @Override
    protected CiProcessId getCiProcessId() {
        return VirtualCiProcessId.toVirtual(
                CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests/a.yaml"), "default-linux-x86_64-release@java"),
                VirtualCiProcessId.VirtualType.VIRTUAL_LARGE_TEST);
    }

    @Override
    protected DiscoveryType getDiscoveryType() {
        return DiscoveryType.STORAGE;
    }

    @Override
    protected void checkPostponeStatus(PostponeLaunch postponeLaunch) {
        var postponeStatus = postponeLaunch.getStatus();
        if (postponeStatus == PostponeStatus.SKIPPED || postponeStatus == PostponeStatus.FAILURE) {
            assertThat(getPostCommitTasks(postponeLaunch.getId()))
                    .describedAs("Status %s must be registered in storage", postponeStatus)
                    .isEqualTo(1);
        } else {
            assertThat(getPostCommitTasks(postponeLaunch.getId()))
                    .describedAs("Status %s must not be registered in storage", postponeStatus)
                    .isEqualTo(0);
        }
    }

    @Test
    void noData() {
        execute();
    }

    @Test
    void progressOnly() {
        discoveryConfig();
        progressDiscovered(COMMIT1);
        execute();
    }

    @Test
    void progressOnlyWithoutCommitAndWithoutLaunches() {
        progressDiscovered(COMMIT1);
        execute();
    }

    @Test
    void progressOnlyWithoutCommit() {
        configure(COMMIT1);
        createLaunch(COMMIT1);

        // Should not happens
        db.currentOrTx(() -> db.arcCommit().delete(ArcCommit.Id.of(COMMIT1.getCommitId())));

        assertThatThrownBy(this::execute)
                .isInstanceOf(NoSuchElementException.class)
                .hasMessage("Unable to find key [ArcCommit.Id(commitId=LargePostCommitServiceTest/r1)] " +
                        "in table [main/Commit]");
    }

    @Test
    void postponeButNotDiscoveredYet() {
        configure(COMMIT1);

        var launch1 = createLaunch(COMMIT1);

        // Cannot start because discovery was stopped at commit0
        Runnable check = () -> {
            execute();
            check(COMMIT1, launch1.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        };

        check.run();

        // Nothing changed
        check.run();
    }

    @Test
    void postponeAndStarted() {
        configure(COMMIT2);

        var launch1 = createLaunch(COMMIT1);

        // Can start, discovered to next commit
        Runnable check = () -> {
            execute();
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        };

        check.run();

        // Nothing changed
        check.run();
    }

    @Test
    void postponeAndStartedAndComplete() {
        configure(COMMIT2);

        var launch1 = createLaunch(COMMIT1);

        // Can start, discovered to next commit
        execute();
        check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);

        completeLaunch(launch1.getLaunchId());
        Runnable check = () -> {
            execute();
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        };
        check.run();

        check.run();

    }

    @Test
    void postponeMultipleButStartOne() {
        configure(COMMIT3);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        // Nothing changed
        check.run();
    }

    @Test
    void postponeMultipleButStartOneAndComplete() {
        configure(COMMIT3);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Can start, discovered to first commit
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleThenStartLast() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            });
        };

        check.run();

        // Nothing changed
        check.run();
    }

    @Test
    void postponeMultipleThenStartLastAndCompleteSequentialAsc() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch3.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleThenStartLastAndCompleteSequentialDesc() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch3.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleThenStartLastAndCompleteAll() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleThenStartLastAndCompleteAllAfterNotReady() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        compareLargeTestsAll(false, false); // Not ready yet
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(true, false); // And ready
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleThenCancelLeft() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        compareLargeTestsAll(false, false); // Not ready yet
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(true, false, true, false); // Canceled left
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleThenCancelRight() {
        configure(COMMIT5);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        compareLargeTestsAll(false, false); // Not ready yet
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(true, false, false, true); // Canceled right
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
        });


        // Yes, this run will be paused until new commit is registered
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
            });
        };
        check.run();

        check.run();


        var launch4 = createLaunch(COMMIT4);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });


        compareLargeTestsAll(true, true, false, false); // Everything is OK, start on new range
        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.CANCELED, DEFAULT);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }


    @Test
    void postponeMultipleThenStartLastAndCompleteAllWithBinarySearch() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        compareLargeTestsAll(false, true); // Not ready yet
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(true, true); // And ready but has failed tests
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        // Even we have failed test - it does not really matter
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }


    @Test
    void postponeMultipleButStartOneThenNext() {
        configure(COMMIT3);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        // Nothing changed
        check.run();

        // Nothing changed
        addEmptyCommit(COMMIT4);
        progressDiscovered(COMMIT4);
        var launch4 = createLaunch(COMMIT4);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        // Now starting new launch
        addEmptyCommit(COMMIT5);
        progressDiscovered(COMMIT5);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        var launch5 = createLaunch(COMMIT5);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });


        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };
        check.run();

        check.run();

    }

    @Test
    void postponeMultipleButStartTwo() {
        configure(COMMIT5);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        // Nothing changed
        check.run();
    }

    @Test
    void postponeAll() {
        configure(COMMIT8);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);
        var launch6 = createLaunch(COMMIT6);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null); // Max launches = 2
            });
        };

        check.run();

        check.run();

        var launch7 = createLaunch(COMMIT7);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null); // Max launches = 2
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null); // Max launches = 2
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch6.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch7.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeAllWithBinarySearch() {
        storageApiTestServer.clear();

        configure(COMMIT8);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);
        var launch6 = createLaunch(COMMIT6);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        check.run();

        var launch7 = createLaunch(COMMIT7);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        compareLargeTestsAll(COMMIT1, COMMIT4, true, true);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch3.getLaunchId());
        compareLargeTestsAll(COMMIT3, COMMIT4, true, false);
        compareLargeTestsAll(COMMIT1, COMMIT3, true, true);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch2.getLaunchId());
        compareLargeTestsAll(COMMIT1, COMMIT2, true, false);
        compareLargeTestsAll(COMMIT2, COMMIT3, true, false);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch6.getLaunchId());
        compareLargeTestsAll(COMMIT4, COMMIT6, true, false);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch7.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }


    @Test
    void postponeAllWithBinarySearchMixedRuns() {
        storageApiTestServer.clear();

        configure(COMMIT8);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);
        var launch6 = createLaunch(COMMIT6);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        check.run();

        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        compareLargeTestsAll(COMMIT1, COMMIT4, false, true); // Not ready
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        compareLargeTestsAll(COMMIT1, COMMIT4, true, true); // Ready
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        var launch7 = createLaunch(COMMIT7);
        execute(); // Nothing really changes, still waiting for launch3
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        compareLargeTestsAll(COMMIT4, COMMIT6, true, true);
        completeLaunch(launch6.getLaunchId());
        execute(); // Make sure we don't start launch2
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        failureLaunch(launch5.getLaunchId());
        execute(); // Make sure we don't start launch2
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, BINARY_SEARCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch3.getLaunchId());
        compareLargeTestsAll(COMMIT3, COMMIT4, true, false);
        compareLargeTestsAll(COMMIT1, COMMIT3, true, true);
        execute(); // Time to schedule launch2
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, BINARY_SEARCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        compareLargeTestsAll(COMMIT1, COMMIT2, true, false);
        compareLargeTestsAll(COMMIT2, COMMIT3, true, false);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, BINARY_SEARCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });


        completeLaunch(launch7.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, BINARY_SEARCH);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeAllWithBinarySearchMultipleIntervals() {
        storageApiTestServer.clear();

        configure(COMMIT11);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);
        var launch6 = createLaunch(COMMIT6);
        var launch7 = createLaunch(COMMIT7);
        var launch8 = createLaunch(COMMIT8);
        var launch9 = createLaunch(COMMIT9);
        var launch10 = createLaunch(COMMIT10);

        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT10, launch10.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        check.run();

        // Not ready
        compareLargeTestsAll(COMMIT1, COMMIT4, false, true);
        compareLargeTestsAll(COMMIT4, COMMIT6, false, true);
        compareLargeTestsAll(COMMIT7, COMMIT10, false, true);

        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch7.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch10.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch6.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        // And it's ready, 3 binary search ranges are active
        compareLargeTestsAll(COMMIT1, COMMIT4, true, true);
        compareLargeTestsAll(COMMIT4, COMMIT6, true, true);
        compareLargeTestsAll(COMMIT7, COMMIT10, true, true);
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.START_SCHEDULED, BINARY_SEARCH);
                check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        // Make sure we don't start launch2 and launch8
        check.run();

        // All comparison results are ready and no new failed tests found
        compareLargeTestsAll(COMMIT1, COMMIT3, true, false);

        completeLaunch(launch3.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(COMMIT7, COMMIT9, true, false);
        completeLaunch(launch9.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT9, launch9.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch5.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT8, launch8.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT9, launch9.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeAllWithBinaryRemoveObsoleteLaunches() {
        storageApiTestServer.clear();

        configure(COMMIT11);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);
        var launch6 = createLaunch(COMMIT6);
        var launch7 = createLaunch(COMMIT7);
        var launch8 = createLaunch(COMMIT8);
        var launch9 = createLaunch(COMMIT9);
        var launch10 = createLaunch(COMMIT10);

        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT10, launch10.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        // Nothing changed
        check.run();

        // Not ready
        compareLargeTestsAll(COMMIT1, COMMIT4, false, true);
        compareLargeTestsAll(COMMIT4, COMMIT6, false, true);
        compareLargeTestsAll(COMMIT7, COMMIT10, false, true);

        completeLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch7.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch10.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch6.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        // And it's ready, 3 binary search ranges are active
        compareLargeTestsAll(COMMIT1, COMMIT4, true, true);
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        // Make sure we don't start launch2 and launch8
        check.run();

        progressDiscovered(COMMIT12);
        execute(); // Stopping obsolete launches
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch3.getLaunchId());
        execute(); // Stopping more obsolete launches
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(COMMIT7, COMMIT10, true, true);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
            check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT8, launch8.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT9, launch9.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        // All comparison results are ready and no new failed tests found
        compareLargeTestsAll(COMMIT7, COMMIT9, true, false);
        completeLaunch(launch9.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null, OBSOLETE_LAUNCH);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT8, launch8.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT9, launch9.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT10, launch10.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }


    @ParameterizedTest
    @ValueSource(strings = {"fuzzing", "msan", "asan"})
    void postponeAllAsSlowLaunches(String suffix) {
        ciProcessId = VirtualCiProcessId.toVirtual(
                CiProcessId.ofFlow(
                        Path.of("ci/demo-project/large-tests/a.yaml"),
                        "default-linux-x86_64-%s@java".formatted(suffix)),
                VirtualType.VIRTUAL_LARGE_TEST);

        configure(COMMIT10);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);
        var launch4 = createLaunch(COMMIT4);
        var launch5 = createLaunch(COMMIT5);
        var launch6 = createLaunch(COMMIT6);

        // Can start, discovered to first commit
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT4, launch4.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT5, launch5.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
                check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            });
        };

        check.run();

        // Nothing changed
        check.run();

        var launch7 = createLaunch(COMMIT7);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT5, launch5.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT6, launch6.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch5.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT5, launch5.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT4, launch4.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT5, launch5.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch7.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT4, launch4.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT5, launch5.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch6.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT4, launch4.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT5, launch5.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT6, launch6.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT7, launch7.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }


    @Test
    void postponeMultipleWithFailure1() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch3.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleWithFailure3() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch3.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleWithFailureAndBinarySearch() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        compareLargeTestsAll(true, true); // Does not matter, binary search won't start
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @Test
    void postponeMultipleWithFailureAndBinarySearchContinue() {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch3.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
        });


        var launch4 = createLaunch(COMMIT4);
        progressDiscovered(COMMIT5);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });


        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch4.getLaunchId());
        compareLargeTestsAll(true, true); // Has new failed tests
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.FAILURE, DEFAULT);
                check(COMMIT4, launch4.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    @ParameterizedTest
    @MethodSource("canceledScenarios")
    void postponeMultipleWithBinarySearchButCanceled(
            boolean cancelLeft,
            boolean cancelRight,
            PostponeStatus statusLeft,
            Status launchStatusMid,
            PostponeStatus statusMid,
            PostponeStatus statusRight
    ) {
        configure(COMMIT4);

        var launch1 = createLaunch(COMMIT1);
        var launch2 = createLaunch(COMMIT2);
        var launch3 = createLaunch(COMMIT3);

        // Start last commit if this is the last commit and discovered commit moved further
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        completeLaunch(launch3.getLaunchId());

        compareLargeTestsAll(false, true); // Not ready yet
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
        });

        compareLargeTestsAll(true, false, cancelLeft, cancelRight); // And ready but canceled on one side
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, statusLeft, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null); // Won't change
            check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, statusRight, DEFAULT);
        });

        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, statusLeft, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), launchStatusMid, statusMid, null); // Now it can be changed
                check(COMMIT3, launch3.getLaunchId(), Status.SUCCESS, statusRight, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    private void execute() {
        binarySearchExecutor.execute(handler);
    }


    protected void compareLargeTestsAll() {
        compareLargeTestsAll(true, false);
    }

    protected void compareLargeTestsAll(boolean ready, boolean hasNewFailedTests) {
        compareLargeTestsAll(ready, hasNewFailedTests, false, false);
    }

    protected void compareLargeTestsAll(
            boolean ready,
            boolean hasNewFailedTests,
            boolean canceledLeft,
            boolean canceledRight
    ) {
        var response = StorageApi.CompareLargeTasksResponse.newBuilder()
                .setCompareReady(ready)
                .setCanceledLeft(canceledLeft)
                .setCanceledRight(canceledRight);
        if (hasNewFailedTests) {
            response.addDiffs(StorageApi.TestDiff.newBuilder()
                    .setDiffType(Common.TestDiffType.TDT_FAILED)
                    .build());
        }
        storageApiTestServer.setDefaultCompareLargeTasks(response.build());
    }

    protected void compareLargeTestsAll(
            ArcCommit left,
            ArcCommit right,
            boolean ready,
            boolean hasNewFailedTests
    ) {
        var response = StorageApi.CompareLargeTasksResponse.newBuilder()
                .setCompareReady(ready);
        if (hasNewFailedTests) {
            response.addDiffs(StorageApi.TestDiff.newBuilder()
                    .setDiffType(Common.TestDiffType.TDT_FAILED)
                    .build());
        }

        var request = StorageApi.CompareLargeTasksRequest.newBuilder()
                .setLeft(largeTaskId(left))
                .setRight(largeTaskId(right))
                .addAllFilterDiffTypes(List.of(
                        Common.TestDiffType.TDT_FAILED,
                        Common.TestDiffType.TDT_FAILED_BROKEN,
                        Common.TestDiffType.TDT_FAILED_NEW
                ))
                .build();
        storageApiTestServer.setCompareLargeTasks(request, response.build());
    }

    protected long getPostCommitTasks(PostponeLaunch.Id id) {
        List<LargePostCommitWriterTask.Params> params = bazingaTaskManagerStub.getJobsParameters(
                TaskId.from(LargePostCommitWriterTask.class));
        return params.stream()
                .map(LargePostCommitWriterTask.Params::toId)
                .filter(id::equals)
                .count();
    }

    static List<Arguments> canceledScenarios() {
        return List.of(
                Arguments.of(
                        true,
                        false,
                        PostponeStatus.CANCELED,
                        Status.CANCELED,
                        PostponeStatus.SKIPPED,
                        PostponeStatus.COMPLETE
                ),
                Arguments.of(
                        false,
                        true,
                        PostponeStatus.COMPLETE,
                        Status.POSTPONE,
                        PostponeStatus.NEW,
                        PostponeStatus.CANCELED
                ),
                Arguments.of(
                        true,
                        true,
                        PostponeStatus.CANCELED,
                        Status.POSTPONE,
                        PostponeStatus.NEW,
                        PostponeStatus.CANCELED
                )
        );
    }

}
