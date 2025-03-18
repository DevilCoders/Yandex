package ru.yandex.ci.storage.tms.cron;

import java.util.stream.Collectors;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.client.arcanum.GroupsDto;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.groups.GroupEntity;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

public class GroupsSyncCronTask extends CronTask {
    private final CiStorageDb db;
    private final ArcanumClient arcanumClient;
    private final Schedule schedule;

    public GroupsSyncCronTask(CiStorageDb db, ArcanumClient arcanumClient, int periodSeconds) {
        this.db = db;
        this.arcanumClient = arcanumClient;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(periodSeconds));
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        var groups = arcanumClient.getGroups().getData().stream().map(this::toEntity).toList();
        this.db.currentOrTx(() -> this.db.groups().bulkUpsert(groups, Integer.MAX_VALUE));
    }

    private GroupEntity toEntity(GroupsDto.Group group) {
        return new GroupEntity(
                new GroupEntity.Id(group.getName()),
                group.getMembers().stream().map(GroupsDto.Member::getName).collect(Collectors.toSet())
        );
    }
}
