package ru.yandex.ci.common.bazinga.monitoring;

import lombok.extern.slf4j.Slf4j;
import org.joda.time.Duration;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.OnetimeTaskSupport;

import static org.assertj.core.api.Assertions.assertThat;

@Component
@Slf4j
public class TestOneTimeWithContextTask extends OnetimeTaskSupport<Person> {

    private HelloService helloService;

    public TestOneTimeWithContextTask(String name) {
        super(new Person(name));
    }

    @Autowired
    public TestOneTimeWithContextTask(HelloService helloService) {
        super(Person.class);
        this.helloService = helloService;
    }

    @Override
    protected void execute(Person person, ExecutionContext context) throws Exception {
        String greating = helloService.makeHello(person.getName());
        assertThat(greating).isEqualTo("Hello, " + person.getName() + "!");
        log.info("Greating: {}", greating);
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
