package ru.yandex.ci.storage.reader.check;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check.SuspiciousAlert;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.reader.check.suspicious.CheckAnalysisRule;

public class CheckAnalysisService {
    private final List<CheckAnalysisRule> rules;

    public CheckAnalysisService(List<CheckAnalysisRule> rules) {
        this.rules = rules;
    }

    public CheckEntity analyzeCheckFinish(CheckEntity check, Collection<CheckIterationEntity> iterations) {
        var nonHeavyIterations = iterations.stream()
                .filter(iteration -> !iteration.isHeavy())
                .toList();

        var alerts = new HashMap<String, SuspiciousAlert>();
        rules.forEach(rule -> rule.analyzeCheck(check, nonHeavyIterations, text -> addAlert(alerts, rule, text)));

        if (!alerts.isEmpty()) {
            return check.toBuilder()
                    .suspiciousAlerts(alerts.values())
                    .build();
        }

        return check;
    }

    public CheckIterationEntity analyzeIterationFinish(CheckIterationEntity iteration) {
        if (iteration.isHeavy()) {
            return iteration;
        }

        var alerts = new HashMap<String, SuspiciousAlert>();
        rules.forEach(rule -> rule.analyzeIteration(iteration, text -> addAlert(alerts, rule, text)));
        if (!alerts.isEmpty()) {
            return iteration.toBuilder()
                    .suspiciousAlerts(alerts.values())
                    .build();
        }

        return iteration;
    }

    private static void addAlert(Map<String, SuspiciousAlert> alerts, CheckAnalysisRule rule, String text) {
        alerts.put(rule.getId(), new SuspiciousAlert(rule.getId(), text));
    }
}
