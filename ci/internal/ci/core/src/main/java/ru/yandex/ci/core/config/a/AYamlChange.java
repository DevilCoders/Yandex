package ru.yandex.ci.core.config.a;

import java.util.List;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.ConfigProblem;

public class AYamlChange {
    private final String dir;
    private final OrderedArcRevision revision;
    private final Type type;
    private final boolean valid;
    private final List<ConfigProblem> problems;

    public AYamlChange(String dir, OrderedArcRevision revision, Type type, List<ConfigProblem> problems) {
        this.dir = dir;
        this.revision = revision;
        this.type = type;
        this.problems = List.copyOf(problems);
        valid = problems.stream().noneMatch(p -> p.getLevel() == ConfigProblem.Level.CRIT);
    }

    public enum Type {
        ADD,
        DELETE,
        MODIFY
    }

    public String getDir() {
        return dir;
    }

    public OrderedArcRevision getRevision() {
        return revision;
    }

    public Type getType() {
        return type;
    }

    public List<ConfigProblem> getProblems() {
        return problems;
    }

    public boolean isValid() {
        return valid;
    }

}
