package ru.yandex.ci.observer.core.db;

import ru.yandex.ci.observer.core.db.model.check.CheckTable;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationsTable;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTasksTable;
import ru.yandex.ci.observer.core.db.model.settings.ObserverSettingsTable;
import ru.yandex.ci.observer.core.db.model.sla_statistics.SlaStatisticsTable;
import ru.yandex.ci.observer.core.db.model.stress_test.StressTestUsedCommitTable;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTracesTable;

public interface CiObserverDbTables {
    CheckTable checks();

    CheckTasksTable tasks();

    ObserverSettingsTable settings();

    CheckIterationsTable iterations();

    CheckTaskPartitionTracesTable traces();

    SlaStatisticsTable slaStatistics();

    StressTestUsedCommitTable stressTestUsedCommitTable();
}
