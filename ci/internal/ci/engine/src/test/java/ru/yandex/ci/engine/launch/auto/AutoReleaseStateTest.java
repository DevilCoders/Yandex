package ru.yandex.ci.engine.launch.auto;

import java.time.Instant;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

class AutoReleaseStateTest {

    @Test
    void isEnabled() {
        assertFalse(
                new AutoReleaseState(false, false, null).isEnabled(ArcBranch.Type.TRUNK)
        );
        assertFalse(
                new AutoReleaseState(false, false, createLastManualAction(true)).isEnabled(ArcBranch.Type.TRUNK)
        );
        assertFalse(
                new AutoReleaseState(false, false, createLastManualAction(false)).isEnabled(ArcBranch.Type.TRUNK)
        );
        assertTrue(
                new AutoReleaseState(true, false, null).isEnabled(ArcBranch.Type.TRUNK)
        );
        assertTrue(
                new AutoReleaseState(true, false, createLastManualAction(true)).isEnabled(ArcBranch.Type.TRUNK)
        );
        assertFalse(
                new AutoReleaseState(true, false, createLastManualAction(false)).isEnabled(ArcBranch.Type.TRUNK)
        );
    }

    private static AutoReleaseSettingsHistory createLastManualAction(boolean enabled) {
        return new AutoReleaseSettingsHistory(
                AutoReleaseSettingsHistory.Id.of("", Instant.parse("2020-01-02T12:00:00.000Z")),
                enabled, "login", "message"
        );
    }

    @Test
    void isEditable() {
        assertFalse(
                new AutoReleaseState(false, false, null).isEnabled(ArcBranch.Type.TRUNK)
        );
        assertFalse(
                new AutoReleaseState(false, false, createLastManualAction(true)).isEditable()
        );
        assertFalse(
                new AutoReleaseState(false, false, createLastManualAction(false)).isEditable()
        );
        assertTrue(
                new AutoReleaseState(true, false, null).isEditable()
        );
        assertTrue(
                new AutoReleaseState(true, false, createLastManualAction(true)).isEditable()
        );
        assertTrue(
                new AutoReleaseState(true, false, createLastManualAction(false)).isEditable()
        );
    }

}
