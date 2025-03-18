package ru.yandex.ci.engine.launch.auto;

import java.nio.file.Path;
import java.util.NoSuchElementException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;

import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.core.launch.PostponeLaunch.StartReason.BINARY_SEARCH;
import static ru.yandex.ci.core.launch.PostponeLaunch.StartReason.DEFAULT;

class ActionPostCommitsTest extends AbstractLargePostCommitsTest {

    @Autowired
    protected ActionPostCommitHandler handler;

    @Override
    protected CiProcessId getCiProcessId() {
        return CiProcessId.ofFlow(Path.of("autocheck/large-tests/a.yaml"), "test-flow");
    }

    @Override
    protected DiscoveryType getDiscoveryType() {
        return DiscoveryType.DIR;
    }

    @Override
    protected void checkPostponeStatus(PostponeLaunch postponeLaunch) {
        // do nothing
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
        failureLaunch(launch3.getLaunchId());

        // FAILURE is a new COMPLETE (for actions)
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
        });

        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
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

        failureLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        failureLaunch(launch3.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        completeLaunch(launch2.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch6.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
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
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
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

        failureLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        var launch7 = createLaunch(COMMIT7);
        execute(); // Nothing really changes, still waiting for launch3
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT6, launch6.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
        });

        failureLaunch(launch6.getLaunchId());
        execute(); // Make sure we don't start launch2
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT6, launch6.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch3.getLaunchId());
        execute(); // Time to schedule launch2
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT6, launch6.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
            check(COMMIT6, launch6.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT7, launch7.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });


        completeLaunch(launch7.getLaunchId());
        check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT5, launch5.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT6, launch6.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
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
            check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        completeLaunch(launch3.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
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
                check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }


    @Test
    void postponeMultipleWithFailure13() {
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
            check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch3.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.CANCELED, PostponeStatus.SKIPPED, null);
                check(COMMIT3, launch3.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
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

        cancelLaunch(launch3.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.CANCELED, DEFAULT);
        });


        var launch4 = createLaunch(COMMIT4);
        progressDiscovered(COMMIT5);
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.CANCELED, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });


        completeLaunch(launch1.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.POSTPONE, PostponeStatus.NEW, null);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.CANCELED, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, DEFAULT);
        });

        failureLaunch(launch4.getLaunchId());
        execute();
        db.currentOrReadOnly(() -> {
            check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
            check(COMMIT2, launch2.getLaunchId(), Status.STARTING, PostponeStatus.STARTED, BINARY_SEARCH);
            check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.CANCELED, DEFAULT);
            check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
        });

        completeLaunch(launch2.getLaunchId());
        Runnable check = () -> {
            execute();
            db.currentOrReadOnly(() -> {
                check(COMMIT1, launch1.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, DEFAULT);
                check(COMMIT2, launch2.getLaunchId(), Status.SUCCESS, PostponeStatus.COMPLETE, BINARY_SEARCH);
                check(COMMIT3, launch3.getLaunchId(), Status.CANCELED, PostponeStatus.CANCELED, DEFAULT);
                check(COMMIT4, launch4.getLaunchId(), Status.FAILURE, PostponeStatus.COMPLETE, DEFAULT);
            });
        };
        check.run();

        check.run();
    }

    private void execute() {
        binarySearchExecutor.execute(handler);
    }

}
