package ru.yandex.ci.flow.spring;

import java.util.concurrent.Semaphore;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.engine.definition.JobBuilderTest;
import ru.yandex.ci.flow.engine.runtime.DeclaredResourcesNotProducedTest;
import ru.yandex.ci.flow.engine.runtime.JobContextProducedResourcesValidationTest;
import ru.yandex.ci.flow.engine.runtime.JobExecutorConstructorWithParametersTest;
import ru.yandex.ci.flow.engine.runtime.OptionalResourcesTest;
import ru.yandex.ci.flow.engine.runtime.PolymorphicJobsResourceInjectionTest;
import ru.yandex.ci.flow.engine.runtime.ProduceInheritanceTest;
import ru.yandex.ci.flow.engine.runtime.ProduceMultipleResourcesTest;
import ru.yandex.ci.flow.engine.runtime.StagedFlowsTest;
import ru.yandex.ci.flow.engine.runtime.TestUpstreamsFlows;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressContextServiceTest;
import ru.yandex.ci.flow.engine.runtime.state.calculator.JobForceSucceededTest;
import ru.yandex.ci.flow.engine.runtime.test_data.autowired_job.AutowiredJob;
import ru.yandex.ci.flow.engine.runtime.test_data.autowired_job.Bean451;
import ru.yandex.ci.flow.engine.runtime.test_data.common.BadInterruptJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.FailingJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.SleepyJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.StuckJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.WaitingForInterruptOnceJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ConvertMultipleRes1ToRes2;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ConvertRes1ToRes2;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ConvertRes2ToRes3;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.JobThatShouldProduceRes1ButFailsInstead;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes0;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes1;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes1AndFail;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes2;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.MultiResource451SumJob;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Producer451Job;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.ProducerDerived451Job;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Resource451DoubleJob;

import static org.mockito.Mockito.mock;

@Configuration
@Import({
        JobBuilderTest.TestJob.class,
        DeclaredResourcesNotProducedTest.InvalidJob.class,
        JobContextProducedResourcesValidationTest.JobThatDeclaresAndProducesMultipleRes1.class,
        JobContextProducedResourcesValidationTest.JobThatDeclaresAndProducesSingleRes1.class,
        JobContextProducedResourcesValidationTest.JobThatDeclaresSingleRes1ButDoesNotProduceAnything.class,
        JobContextProducedResourcesValidationTest.JobThatDeclaresSingleRes1ButProducesMultipleRes1.class,
        JobContextProducedResourcesValidationTest.JobThatProducesUndeclaredResource.class,
        JobExecutorConstructorWithParametersTest.SomeJob.class,
        OptionalResourcesTest.OptionalResourceJob.class,
        PolymorphicJobsResourceInjectionTest.ChildJob.class,
        PolymorphicJobsResourceInjectionTest.ParentJob.class,
        ProduceInheritanceTest.InheritedJob.class,
        ProduceInheritanceTest.InheritedJobWithTwoResourceProduces.class,
        ProduceMultipleResourcesTest.TwoResourcesJob.class,
        ProduceMultipleResourcesTest.ZeroResourcesJob.class,
        StagedFlowsTest.OnceFailingJob.class,
        TestUpstreamsFlows.ConsumeRes123.class,
        TestUpstreamsFlows.ProduceRes1.class,
        TestUpstreamsFlows.ProduceRes2.class,
        TestUpstreamsFlows.ProduceRes3.class,
        JobProgressContextServiceTest.TestJobExecutor.class,
        JobForceSucceededTest.FailedJob.class,
        AutowiredJob.class,
        BadInterruptJob.class,
        FailingJob.class,
        SleepyJob.class,
        StuckJob.class,
        WaitingForInterruptOnceJob.class,
        ConvertMultipleRes1ToRes2.class,
        ConvertRes1ToRes2.class,
        ConvertRes2ToRes3.class,
        JobThatShouldProduceRes1ButFailsInstead.class,
        ProduceRes0.class,
        ProduceRes1.class,
        ProduceRes1AndFail.class,
        ProduceRes2.class,
        MultiResource451SumJob.class,
        Producer451Job.class,
        ProducerDerived451Job.class,
        Resource451DoubleJob.class
})
public class TestJobsConfig {
    // Some test beans

    @Bean
    public Bean451 bean451() {
        return new Bean451();
    }

    @Bean
    public Semaphore semaphore() {
        return new Semaphore(0, true);
    }

    @Bean
    public Runnable mockRunnable() {
        return mock(Runnable.class);
    }
}
