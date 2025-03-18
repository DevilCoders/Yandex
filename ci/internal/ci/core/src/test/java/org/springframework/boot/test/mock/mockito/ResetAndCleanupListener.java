package org.springframework.boot.test.mock.mockito;

import java.util.Set;

import org.mockito.Mockito;
import org.mockito.internal.util.MockUtil;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.test.context.TestContext;
import org.springframework.test.context.TestExecutionListener;

import ru.yandex.ci.util.Clearable;

public class ResetAndCleanupListener implements TestExecutionListener {

    @Override
    public void afterTestMethod(TestContext testContext) {
        var context = (ConfigurableApplicationContext) testContext.getApplicationContext();
        do {
            var beanFactory = context.getBeanFactory();
            var singletons = Set.of(beanFactory.getSingletonNames());
            for (var name : context.getBeanDefinitionNames()) {
                var definition = beanFactory.getBeanDefinition(name);
                if (definition.isSingleton() && singletons.contains(name)) {
                    Object bean = beanFactory.getSingleton(name);
                    if (MockUtil.isMock(bean)) {
                        if (MockReset.get(bean) == MockReset.NONE) { // Non injected with Spring, just Mocked
                            Mockito.reset(bean);
                        }
                    }
                    if (bean instanceof Clearable clearable) {
                        clearable.clear();
                    }
                }
            }
            if (context.getParent() instanceof ConfigurableApplicationContext) {
                context = (ConfigurableApplicationContext) context.getParent();
            } else {
                context = null;
            }
        } while (context != null);
    }
}
