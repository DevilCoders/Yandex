package ru.yandex.ci.storage.tms.cron;

import java.util.ArrayList;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.kikimr.yql.YqlLimit;
import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.oldci.OldCiClient;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_mute.OldCiMuteActionEntity;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

@Slf4j
public class OldCiMuteCron extends CronTask {
    private final CiStorageDb db;
    private final OldCiClient oldCiClient;
    private final Schedule schedule;

    public OldCiMuteCron(CiStorageDb db, OldCiClient oldCiClient, int periodSeconds) {
        this.db = db;
        this.oldCiClient = oldCiClient;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(periodSeconds));
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        var actions = new ArrayList<OldCiMuteActionEntity>();
        db.scan().run(() -> {
            actions.addAll(db.oldCiMuteActionTable().find(YqlLimit.range(0, 1000)));
        });


        actions.parallelStream().forEach(
                action -> {
                    var testId = action.getTestId();

                    try {
                        execute(action, testId);
                    } catch (HttpException e) {
                        if (e.getHttpCode() == 404) {
                            log.warn("Test not found {}", testId);
                        } else {
                            log.error("General error for test {}", testId, e);
                            return;
                        }
                    }

                    this.db.tx(() -> this.db.oldCiMuteActionTable().delete(action.getId()));
                }
        );
    }

    private void execute(OldCiMuteActionEntity action, String testId) {
        if (action.isMuted()) {
            log.info("Muting {}", action.getTestId());
            oldCiClient.muteTest(testId, action.getToolchain(), action.getReason());
        } else {
            log.info("Unmuting {}", action.getTestId());
            oldCiClient.unmuteTest(testId, action.getToolchain(), action.getReason());
        }
    }
}
