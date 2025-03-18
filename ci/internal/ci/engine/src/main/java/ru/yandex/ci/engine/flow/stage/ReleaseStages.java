package ru.yandex.ci.engine.flow.stage;

import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.definition.stage.StageBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;

/**
 * Описывает типичные релизные стадии, которые подойдут большинству флоу.
 */
public class ReleaseStages extends StageGroup {
    public static final String PRECHECK = "precheck";
    public static final String BUILD = "build";
    public static final String TESTING = "testing";
    public static final String PRESTABLE = "prestable";
    public static final String STABLE = "stable";

    public static final ReleaseStages INSTANCE = new ReleaseStages();

    protected ReleaseStages() {
        super(
                StageBuilder.create(PRECHECK),
                StageBuilder.create(BUILD),
                StageBuilder.create(TESTING),
                StageBuilder.create(PRESTABLE),
                StageBuilder.create(STABLE).uninterruptable()
        );
    }

    public Stage precheck() {
        return getStage(PRECHECK);
    }

    public Stage build() {
        return getStage(BUILD);
    }

    public Stage testing() {
        return getStage(TESTING);
    }

    public Stage prestable() {
        return getStage(PRESTABLE);
    }

    public Stage stable() {
        return getStage(STABLE);
    }
}
