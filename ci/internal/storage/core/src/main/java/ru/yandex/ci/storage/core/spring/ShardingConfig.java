package ru.yandex.ci.storage.core.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;

@Configuration
@Import({
        StorageYdbConfig.class
})
public class ShardingConfig {
    @Bean
    public ShardingSettings shardingSettings(
            @Value("${storage.shardingSettings.configureChunks}") int configureChunks,
            @Value("${storage.shardingSettings.buildChunks}") int buildChunks,
            @Value("${storage.shardingSettings.styleChunks}") int styleChunks,
            @Value("${storage.shardingSettings.smallTestChunks}") int smallTestChunks,
            @Value("${storage.shardingSettings.mediumTestChunks}") int mediumTestChunks,
            @Value("${storage.shardingSettings.largeTestChunks}") int largeTestChunks,
            @Value("${storage.shardingSettings.teTestChunks}") int teTestChunks,
            @Value("${storage.shardingSettings.nativeBuildChunks}") int nativeBuildsChunks,
            @Value("${storage.shardingSettings.chunkSamplingPercent}") int chunkSamplingPercent,
            @Value("${storage.shardingSettings.numberOfChecksToSkip}") int numberOfChecksToSkip
    ) {
        return new ShardingSettings(
                configureChunks,
                buildChunks,
                styleChunks,
                smallTestChunks,
                mediumTestChunks,
                largeTestChunks,
                teTestChunks,
                nativeBuildsChunks,
                chunkSamplingPercent,
                numberOfChecksToSkip
        );
    }

    @Bean(initMethod = "initializeOrCheckState")
    public ChunkDistributor chunkDistributor(
            CiStorageDb db,
            ShardingSettings shardingSettings,
            @Value("${storage.chunkDistributor.numberOfPartitions}") int numberOfPartitions
    ) {
        return new ChunkDistributor(db, shardingSettings, numberOfPartitions);
    }
}
