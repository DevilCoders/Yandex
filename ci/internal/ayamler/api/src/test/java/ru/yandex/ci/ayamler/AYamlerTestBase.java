package ru.yandex.ci.ayamler;

import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.ayamler.api.spring.AYamlerTestConfig;

@ContextConfiguration(classes = AYamlerTestConfig.class)
public class AYamlerTestBase extends CommonTestBase {
}
