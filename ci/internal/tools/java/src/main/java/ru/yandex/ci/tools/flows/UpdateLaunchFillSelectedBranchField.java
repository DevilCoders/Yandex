package ru.yandex.ci.tools.flows;

import java.util.ArrayList;
import java.util.HashSet;

import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.mutable.MutableInt;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(YdbCiConfig.class)
public class UpdateLaunchFillSelectedBranchField extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Override
    protected void run() {
        var read = new MutableInt(0);
        var launchIds = new ArrayList<Launch.Id>(1_000_000);
        db.currentOrReadOnly(() ->
                {
                    var launchStream = db.launches().streamAll(1000);
                    boolean stop = false;
                    for (var iter = launchStream.iterator(); iter.hasNext() && !stop; ) {
                        var launch = iter.next();

                        read.increment();
                        if (launch.getSelectedBranch() == null) {
                            launchIds.add(launch.getId());
                        }
                        if (read.getValue() % 10000 == 0) {
                            log.info("Read {}, launchIds {}", read.getValue(), launchIds.size());
                        }
                        stop = launchIds.size() > 500_000;
                    }
                }
        );

        var i = new MutableInt(1);
        for (var ids : Lists.partition(launchIds, 999)) {
            db.currentOrTx(() -> {
                try {
                    log.info("Started batch {}", i.getValue());
                    db.launches().find(new HashSet<>(ids))
                            .stream()
                            .map(it -> it.toBuilder()
                                    .vcsInfo(it.getVcsInfo())
                                    .build()
                            )
                            .forEach(launch -> db.launches().save(launch));
                    log.info("Finished batch {}", i.getValue());
                } catch (Exception e) {
                    log.info("Failed batch {}: ids {}", i.getValue(), ids, e);
                }
            });
            i.increment();
        }
    }

    // launch the method with something like `-Xms10G -Xmx12G`
    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
