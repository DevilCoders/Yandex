package ru.yandex.ci.common.ydb.spring;

import java.util.Properties;

import javax.sql.DataSource;

import com.yandex.ydb.jdbc.YdbDriver;
import com.yandex.ydb.jdbc.settings.YdbConnectionProperty;
import com.yandex.ydb.spring.data.YdbDataSourceTransactionManager;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Lazy;
import org.springframework.jdbc.datasource.SimpleDriverDataSource;
import org.springframework.transaction.PlatformTransactionManager;

import yandex.cloud.repository.kikimr.KikimrConfig;

import ru.yandex.ci.common.ydb.YdbExecutor;

@Lazy
@Configuration
@Import(YdbConfig.class)
public class YdbJdbcConfig {

    @Bean
    public DataSource ydbDataSource(KikimrConfig kikimrConfig) {
        var endpoints = kikimrConfig.getEndpoints();
        if (endpoints == null || endpoints.isEmpty()) {
            throw new IllegalStateException("Unable to parse KikimrConfig, no endpoints present");
        }
        var hostAndPort = endpoints.get(0);
        var url = "jdbc:ydb:%s:%s".formatted(hostAndPort.getHost(), hostAndPort.getPort());

        var props = new Properties();
        props.setProperty(YdbConnectionProperty.DATABASE.getName(), kikimrConfig.getTablespace()); // Yeah, cloud ORM...

        var kikimrAuth = kikimrConfig.getAuth();
        if (kikimrAuth != null) {
            switch (kikimrAuth.getType()) {
                case NONE -> {
                }
                case TOKEN -> props.setProperty(YdbConnectionProperty.TOKEN.getName(), kikimrAuth.getToken());
                case TOKEN_FILE -> props.setProperty(YdbConnectionProperty.TOKEN.getName(), kikimrAuth.getTokenFile());
                default -> throw new IllegalStateException("Unsupported auth type: " + kikimrAuth.getType());
            }
        }
        return new SimpleDriverDataSource(new YdbDriver(), url, props);
    }

    @Bean
    public PlatformTransactionManager ydbTransactionManager(DataSource ydbDataSource) {
        return new YdbDataSourceTransactionManager(ydbDataSource);
    }

    @Bean
    public YdbExecutor ydbExecutor(PlatformTransactionManager ydbTransactionManager) {
        return new YdbExecutor(ydbTransactionManager);
    }
}
