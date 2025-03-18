package ru.yandex.ci.tms.task.autocheck.degradation;

import java.util.OptionalInt;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.tms.data.autocheck.SemaphoreId;

public class AutocheckDegradationStateKeeper {
    private static final String NAMESPACE = "AutocheckDegradationState";
    private static final String PREVIOUS_TRIGGER_STATE_NAME = "prevTriggerState";

    private final CiDb db;

    public AutocheckDegradationStateKeeper(CiDb db) {
        this.db = db;
    }

    public void setSemaphoreValue(SemaphoreId semaphoreId, int value) {
        db.currentOrTx(() -> db.keyValue().setValue(
                NAMESPACE, semaphoreId.getStringId(), value
        ));
    }

    public OptionalInt getSemaphoreValue(SemaphoreId semaphoreId) {
        return db.currentOrReadOnly(
                () -> db.keyValue().findInt(NAMESPACE, semaphoreId.getStringId())
        );
    }

    public void setPreviousTriggerState(boolean triggerState) {
        db.currentOrTx(
                () -> db.keyValue().setValue(NAMESPACE, PREVIOUS_TRIGGER_STATE_NAME, triggerState)
        );
    }

    public boolean getPreviousTriggerState() {
        return db.currentOrReadOnly(
                () -> db.keyValue().getBoolean(NAMESPACE, PREVIOUS_TRIGGER_STATE_NAME, false)
        );
    }
}
