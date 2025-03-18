package ru.yandex.ci.tools.testenv.migration;

import java.util.List;

import javax.sql.DataSource;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.datasource.DriverManagerDataSource;

import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.engine.spring.clients.TestenvClientConfig;
import ru.yandex.ci.engine.spring.clients.TrackerClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.startrek.client.Session;

@Import({
        TestenvMigrationApplication.Config.class,
        TestenvClientConfig.class,
        TrackerClientConfig.class
})
@Configuration
public class TestenvMigrationApplication extends AbstractSpringBasedApp {

    @Autowired
    private TestenvMigrationService service;

    @Override
    protected void run() throws Exception {
//        service.stopEmptyDatabases(); //Long...
        service.processProjects();

        service.stopInactiveRmComponents(List.of(
        ));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

    @Configuration
    @PropertySource(value = "file:${user.home}/.ci/ci-local.properties", ignoreResourceNotFound = true)
    public static class Config {


        @Bean
        public Session trackerSession(TrackerClient trackerClient, @Value("${ci.startrek.oauth}") String token) {
            return trackerClient.getSession(token);
        }

        @Bean
        public TestenvMigrationService testenvMigrationService(
                JdbcTemplate jdbcTemplate,
                TestenvClient testenvClient,
                Session trackerSession
        ) {
            return new TestenvMigrationService(jdbcTemplate, testenvClient, trackerSession);
        }

        @Bean
        public static DataSource testenvSystemDbDataSource(@Value("${ci.testenv.db.system.password}") String password) {
            var dataSource = new DriverManagerDataSource();
            dataSource.setDriverClassName("com.mysql.jdbc.Driver");
            dataSource.setUrl("jdbc:mysql://c-mdb4ti8a84ct89f4l27a.rw.db.yandex.net:3306/testenv?useSSL=true");
            dataSource.setUsername("testenv");
            dataSource.setPassword(password);

            return dataSource;
        }

        @Bean
        public static JdbcTemplate jdbcTemplate(DataSource testenvSystemDbDataSource) {
            return new JdbcTemplate(testenvSystemDbDataSource);
        }


    }


}
