package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(YdbCiConfig.class)
public class LoadPullRequests extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Override
    protected void run() {
        var list = db.currentOrReadOnly(() -> db.pullRequestDiffSetTable().suggestPullRequestId(null, 0, 21));
        list.forEach(pullRequestId -> log.info("PR: {}", pullRequestId));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
