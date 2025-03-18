package ru.yandex.ci.tools.yamls;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.ResourceUtils;

@Slf4j
@Configuration
public class ValidateSingleAYaml extends AbstractSpringBasedApp {

    @Override
    protected void run() throws JsonProcessingException, ProcessingException {
        var text = ResourceUtils.textResource("y.yaml");
        var ret = AYamlParser.parseAndValidate(text);
        for (var error : ret.getStaticErrors()) {
            log.info("{}", error);
        }

        for (var report : ret.getSchemaReportMessages()) {
            log.info("{}", report);
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
