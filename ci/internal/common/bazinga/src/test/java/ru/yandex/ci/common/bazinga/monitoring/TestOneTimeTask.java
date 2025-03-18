package ru.yandex.ci.common.bazinga.monitoring;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.joda.time.Duration;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import ru.yandex.commune.bazinga.scheduler.ActiveUidBehavior;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDropType;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDuplicateBehavior;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.OnetimeTaskSupport;

@Component
public class TestOneTimeTask extends OnetimeTaskSupport<Person> {
    private final List<String> calls = new CopyOnWriteArrayList<>();

    public TestOneTimeTask(String value) {
        super(new Person(value));
    }

    @Autowired
    public TestOneTimeTask() {
        super(Person.class);
    }

    @Override
    protected void execute(Person parameters, ExecutionContext context) {
        calls.add(parameters.getName());
    }

    @Override
    public ActiveUidBehavior activeUidBehavior() {
        return new ActiveUidBehavior(ActiveUidDropType.WHEN_FINISHED, ActiveUidDuplicateBehavior.MERGE);
    }

    public List<String> getCalls() {
        return List.copyOf(calls);
    }

    public void reset() {
        calls.clear();
    }

    @Override
    public int priority() {
        return 0;
    }

    @Override
    public Duration timeout() {
        return Duration.standardSeconds(1);
    }
}
