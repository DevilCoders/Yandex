package ru.yandex.ci.engine.autocheck.jobs.autocheck.jobs;

import java.util.Collections;

import com.google.gson.JsonObject;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.autocheck.FlowVars;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.CiStorageIntegrationTestBase;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGenerator;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;

@Slf4j
public class StartAutocheckJobIntegrationTest extends CiStorageIntegrationTestBase {

    @Test
    void execute() throws Exception {
        log.info("storageApiController={}", storageApiController);

        var checksBefore = apiCheckService.findChecksByRevisions("r2", "r3", Collections.emptySet());
        assertThat(checksBefore).isEmpty();

        //need for register new checks
        CheckIdGenerator.fillDb(ciStorageDb, 10);

        setAutocheckEnabled();

        var flowVars = new JsonObject();
        flowVars.addProperty(FlowVars.IS_PRECOMMIT, false);
        var jobContext = createJobContext(TRUNK_POSTCOMMIT_PROCESS_ID, flowVars);
        startAutocheckJob.execute(jobContext);

        var checksAfter = apiCheckService.findChecksByRevisions("r2", "r3", Collections.emptySet());
        log.info("after: " + checksAfter);
        assertThat(checksAfter.size()).isEqualTo(1);
    }

}
