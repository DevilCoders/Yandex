package ru.yandex.ci.engine.event;

import java.time.Duration;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;
import ru.yandex.misc.codec.Hex;

@NonNullApi
public class LaunchEventTask extends AbstractOnetimeTask<LaunchEventTask.Params> {

    private LogbrokerWriter logbrokerLaunchEventWriter;

    public LaunchEventTask(LogbrokerWriter logbrokerLaunchEventWriter) {
        super(LaunchEventTask.Params.class);
        this.logbrokerLaunchEventWriter = logbrokerLaunchEventWriter;
    }

    public LaunchEventTask(LaunchId launchId, LaunchState.Status launchStatus, byte[] data) {
        super(new Params(launchId, launchStatus, data));
    }

    @Override
    protected void execute(LaunchEventTask.Params params, ExecutionContext context) throws Exception {
        byte[] data = params.getData();
        if (data.length > 0) {
            this.logbrokerLaunchEventWriter.write(data).get();
        }
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @BenderBindAllFields
    public static class Params {
        private final String ciProcessId;
        private final int number;
        private final LaunchState.Status launchStatus;
        private final String data;

        public Params(LaunchId launchId, LaunchState.Status launchStatus, byte[] data) {
            this.ciProcessId = launchId.getProcessId().asString();
            this.number = launchId.getNumber();
            this.launchStatus = launchStatus;
            this.data = Hex.encode(data);
        }

        public LaunchId getLaunchId() {
            return LaunchId.of(CiProcessId.ofString(ciProcessId), number);
        }

        public LaunchState.Status getLaunchStatus() {
            return launchStatus;
        }

        public byte[] getData() {
            return Hex.decode(data);
        }
    }

}
