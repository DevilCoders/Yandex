package ru.yandex.ci.storage.tms.spring.bazinga;

import org.springframework.beans.BeansException;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.config.ConfigurableListableBeanFactory;
import org.springframework.beans.factory.support.BeanDefinitionRegistry;
import org.springframework.beans.factory.support.BeanDefinitionRegistryPostProcessor;
import org.springframework.beans.factory.support.RootBeanDefinition;
import org.springframework.boot.web.servlet.ServletRegistrationBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.Option;
import ru.yandex.commune.admin.web.AdminApp;
import ru.yandex.commune.admin.web.AdminBender;
import ru.yandex.commune.admin.web.AdminConfigurator;
import ru.yandex.commune.admin.web.support.AdminMultilineTextTemplateEngine;
import ru.yandex.commune.util.serialize.ToMultilineSerializer;
import ru.yandex.commune.util.serialize.ToMultilineSerializerContextConfiguration;
import ru.yandex.misc.env.EnvironmentType;

@Configuration
@Import(ToMultilineSerializerContextConfiguration.class)
public class StorageZAdminConfig implements BeanDefinitionRegistryPostProcessor {

    @Autowired
    private ToMultilineSerializer toMultilineSerializer;

    private AdminApp adminApp;

    @Override
    public void postProcessBeanDefinitionRegistry(BeanDefinitionRegistry registry) throws BeansException {
        adminApp = AdminConfigurator.configure(
                "/z",
                (uri, servlet) -> {

                    registry.registerBeanDefinition(
                            servlet.getClass().getCanonicalName(),
                            new RootBeanDefinition(
                                    ServletRegistrationBean.class,
                                    () -> new ServletRegistrationBean<>(servlet, uri)
                            )
                    );
                },
                (uri, filter) -> {
                },
                AdminBender.mapper,
                Cf.list(new AdminMultilineTextTemplateEngine(AdminBender.mapper, toMultilineSerializer)),
                Option.empty(),
                Option.of(EnvironmentType.getActive()
                )
        );
    }

    @Bean
    public AdminApp adminApp() {
        return adminApp;
    }

    @Override
    public void postProcessBeanFactory(ConfigurableListableBeanFactory beanFactory) throws BeansException {
    }

}
