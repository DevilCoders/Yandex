package ru.yandex.ci.common.bazinga.monitoring;

import org.springframework.stereotype.Component;

@Component
public class HelloService {
    public String makeHello(String name) {
        return "Hello, " + name + "!";
    }
}
