package ru.yandex.ci.flow.engine.runtime;

import javax.annotation.Nonnull;

import com.google.common.base.Strings;
import com.google.gson.JsonPrimitive;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.flow.engine.definition.context.JobContext;

@Slf4j
@RequiredArgsConstructor
public class JobConditionalChecks {

    @Nonnull
    private final TaskletContextProcessor taskletContextProcessor;

    public boolean shouldRun(@Nonnull JobContext jobContext) {

        var jobState = jobContext.getJobState();
        var expression = jobState.getConditionalRunExpression();
        if (Strings.isNullOrEmpty(expression)) {
            return true; // ---
        }

        try {
            return shouldRunImpl(expression, jobContext);
        } catch (Exception e) {
            throw new ConditionalRuntimeException(e.getMessage(), e);
        }
    }

    private boolean shouldRunImpl(String expression, JobContext jobContext) {
        var jobState = jobContext.getJobState();
        var jobId = jobState.getJobId();

        log.info("Job [{}] must be evaluated for conditional execution: {}", jobId, expression);
        var documentSource = taskletContextProcessor.getDocumentSource(jobContext);

        // No caching or anything - in most cases job should be checked just once
        var result = PropertiesSubstitutor.substitute(new JsonPrimitive(expression), documentSource);
        var shouldRun = PropertiesSubstitutor.asBoolean(result);
        log.info("Job [{}] result evaluated, should run = {}, based on result [{}]", jobId, shouldRun, result);
        return shouldRun;
    }
}
