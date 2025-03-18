package ru.yandex.ci.storage.tms.spring.bazinga;

import javax.annotation.PostConstruct;

import org.eclipse.jetty.server.handler.ErrorHandler;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.commune.admin.web.AdminApp;
import ru.yandex.commune.bazinga.BazingaConfiguration;
import ru.yandex.commune.bazinga.BazingaControllerAndWorkerApps;
import ru.yandex.commune.bazinga.admin.BazingaAdminAddressResolver;
import ru.yandex.commune.bazinga.admin.BazingaAdminPageConfigurator;
import ru.yandex.commune.bazinga.admin.BazingaMasterHolder;
import ru.yandex.commune.bazinga.admin.BazingaMasterListener;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.util.serialize.ToMultilineSerializer;
import ru.yandex.commune.util.serialize.ToMultilineSerializerContextConfiguration;
import ru.yandex.commune.zk2.admin.ZkAdminContextConfiguration;
import ru.yandex.commune.zk2.client.ZkManager;

@Configuration
@Import({
        ToMultilineSerializerContextConfiguration.class,
        StorageBazingaServiceConfig.class,
        StorageZAdminConfig.class,
        ZkAdminContextConfiguration.class,
        S3LogStorageConfig.class
})
public class StorageBazingaAdminConfig {

    @Autowired
    private AdminApp adminApp;

    @Autowired
    private BazingaAdminAddressResolver bazingaAdminAddressResolver;

    @Autowired
    private BazingaConfiguration bazingaConfiguration;

    @Autowired
    private BazingaStorage bazingaStorage;

    @Autowired
    private ToMultilineSerializer toMultilineSerializer;

    @Autowired
    private ZkManager zkManager;

    @Autowired
    private BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps;

    @Autowired
    private S3LogStorage logStorageS3;

    @Bean
    public ServletContextHandler servletContextHandler() {
        ServletContextHandler handler = new ServletContextHandler(
                null, "/", ServletContextHandler.SESSIONS
        );

        handler.setErrorHandler(new ErrorHandler());
        return handler;
    }

    @PostConstruct
    public void configure() {
        var holder = new BazingaMasterHolder();
        var listener = new BazingaMasterListener(holder);

        BazingaAdminPageConfigurator.configure(
                adminApp,
                bazingaConfiguration,
                bazingaStorage,
                bazingaAdminAddressResolver,
                toMultilineSerializer,
                zkManager,
                holder,
                listener,
                Option.ofNullable(bazingaControllerAndWorkerApps.getControllerApp()),
                Option.ofNullable(bazingaControllerAndWorkerApps.getWorkerApp()),
                Option.ofNullable(logStorageS3)
        );
    }


}
