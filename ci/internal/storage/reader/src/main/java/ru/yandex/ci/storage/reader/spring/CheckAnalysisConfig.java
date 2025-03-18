package ru.yandex.ci.storage.reader.spring;

import java.util.List;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.reader.check.CheckAnalysisService;
import ru.yandex.ci.storage.reader.check.suspicious.CheckAnalysisRule;
import ru.yandex.ci.storage.reader.check.suspicious.MoreTestsDeletedThanAddedRule;
import ru.yandex.ci.storage.reader.check.suspicious.NewOwnedFlakyRule;
import ru.yandex.ci.storage.reader.check.suspicious.RightTimeoutsRule;

@Configuration
@Import({StorageYdbConfig.class})
public class CheckAnalysisConfig {

    @Bean
    public CheckAnalysisService checkAnalysisService(
            @Qualifier("suspiciousCheck") List<CheckAnalysisRule> suspiciousCheckRules
    ) {
        return new CheckAnalysisService(suspiciousCheckRules);
    }

    @Bean
    @Qualifier("suspiciousCheck")
    public CheckAnalysisRule manyTestsDeletedRule(
            CiStorageDb db,
            @Value("${storage.manyTestsDeletedRule.deletedPercent}") int deletedPercent,
            @Value("${storage.manyTestsDeletedRule.deletedMin}") int deletedMin
    ) {
        return new MoreTestsDeletedThanAddedRule(db, deletedPercent, deletedMin);
    }

    @Bean
    @Qualifier("suspiciousCheck")
    public CheckAnalysisRule rightTimeoutsRule(
            @Value("${storage.rightTimeoutsRule.rightPercent}") int rightPercent,
            @Value("${storage.rightTimeoutsRule.totalPercent}") int totalPercent
    ) {
        return new RightTimeoutsRule(totalPercent, rightPercent);
    }

    @Bean
    @Qualifier("suspiciousCheck")
    public CheckAnalysisRule newOwnedFlakysRule(
            CiStorageDb db
    ) {
        return new NewOwnedFlakyRule(db);
    }
}
