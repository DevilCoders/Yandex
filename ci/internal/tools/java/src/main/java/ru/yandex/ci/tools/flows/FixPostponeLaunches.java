package ru.yandex.ci.tools.flows;

import java.util.ArrayList;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.spring.UrlServiceConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import({
        YdbCiConfig.class,
        UrlServiceConfig.class
})
public class FixPostponeLaunches extends AbstractSpringBasedApp {
    private final String expectTid = System.getenv("TID_TOKEN");
    private final YavToken.Id targetTokenId = YavToken.Id.of(expectTid);

    @Autowired
    CiDb db;

    @Autowired
    UrlService urlService;

    @Override
    protected void run() {
        var started = db.currentOrReadOnly(() ->
                db.postponeLaunches().findAll(
                        VirtualType.VIRTUAL_LARGE_TEST, // Lookup for virtual large tests only (for now)
                        ArcBranch.trunk(),
                        PostponeStatus.STARTED
                ));

        var urls = new ArrayList<String>();
        for (var postpone : started) {
            db.currentOrTx(() -> {
                var launch = db.launches().get(postpone.toLaunchId());
                if (launch.getStatus() == LaunchState.Status.FAILURE) {
                    var path = launch.getProcessId().getPath();
                    if (path.startsWith("large-test@devtools/ymake")) {
                        var launchRuntime = launch.getFlowInfo().getRuntimeInfo();
                        if ("DEVTOOLS-LARGE".equals(launchRuntime.getSandboxOwner()) &&
                                !expectTid.equals(launchRuntime.getYavTokenUidOrThrow().getId())) {
                            updateLaunch(launch);
                            updateFlowLaunch(db.flowLaunch().get(FlowLaunchId.of(launch.getLaunchId())));
                            urls.add(urlService.toLaunch(launch));
                            return;
                        }
                    }
                    urls.add(urlService.toLaunch(launch));
                }
            });
        }

        for (var url : urls) {
            log.error("Failure launch: {}", url);
        }
    }

    private void updateLaunch(Launch launch) {
        var launchFlowInfo = launch.getFlowInfo();
        var launchRuntime = launchFlowInfo.getRuntimeInfo();
        db.launches().save(
                launch.toBuilder()
                        .flowInfo(launchFlowInfo.toBuilder()
                                .runtimeInfo(launchRuntime.toBuilder()
                                        .yavTokenUid(targetTokenId)
                                        .build())
                                .build())
                        .build());

    }

    private void updateFlowLaunch(FlowLaunchEntity flowLaunch) {
        var flowFlowInfo = flowLaunch.getFlowInfo();
        var flowRuntime = flowFlowInfo.getRuntimeInfo();
        db.flowLaunch().save(
                flowLaunch.toBuilder()
                        .flowInfo(flowFlowInfo.toBuilder()
                                .runtimeInfo(flowRuntime.toBuilder()
                                        .yavTokenUid(targetTokenId)
                                        .build())
                                .build())
                        .build());
    }


    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
