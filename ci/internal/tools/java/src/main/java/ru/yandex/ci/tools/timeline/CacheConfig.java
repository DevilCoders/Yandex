package ru.yandex.ci.tools.timeline;

import java.io.File;
import java.io.Serializable;
import java.nio.file.Paths;
import java.util.List;

import javax.cache.CacheManager;
import javax.cache.Caching;
import javax.cache.spi.CachingProvider;

import org.ehcache.config.CacheConfiguration;
import org.ehcache.config.builders.CacheConfigurationBuilder;
import org.ehcache.config.builders.ResourcePoolsBuilder;
import org.ehcache.config.units.MemoryUnit;
import org.ehcache.core.config.DefaultConfiguration;
import org.ehcache.expiry.ExpiryPolicy;
import org.ehcache.impl.config.persistence.DefaultPersistenceConfiguration;
import org.ehcache.jsr107.Eh107Configuration;
import org.ehcache.jsr107.EhcacheCachingProvider;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class CacheConfig {

    private static final File CACHE_PATH =
            Paths.get(System.getProperty("user.home"), "Desktop", "ci-tool-cache").toFile();

    @Bean
    public CacheManager jCacheCacheManager(List<String> cacheNames) {
        CachingProvider provider = Caching.getCachingProvider();
        var ehcacheProvider = (EhcacheCachingProvider) provider;
        DefaultConfiguration configuration = new DefaultConfiguration(ehcacheProvider.getDefaultClassLoader(),
                new DefaultPersistenceConfiguration(CACHE_PATH)
        );

        javax.cache.CacheManager cacheManager = ehcacheProvider.getCacheManager(
                ehcacheProvider.getDefaultURI(),
                configuration
        );

        for (String cacheName : cacheNames) {
            var cacheConfiguration = Eh107Configuration.fromEhcacheCacheConfiguration(defaultCacheConfiguration());
            cacheManager.createCache(cacheName, cacheConfiguration);
        }
        return cacheManager;
    }

    @Bean
    public CacheConfiguration<?, ?> defaultCacheConfiguration() {
        return CacheConfigurationBuilder.newCacheConfigurationBuilder(Serializable.class, Object.class,
                ResourcePoolsBuilder.newResourcePoolsBuilder()
                        .disk(1, MemoryUnit.GB, true)
                        .heap(300, MemoryUnit.MB)
                        .build()
        )
                .withValueSerializer(GsonSerializer.asTypedSerializer())
                .withKeySerializer(GsonSerializer.asTypedSerializer())
                .withExpiry(ExpiryPolicy.NO_EXPIRY)
                .build();
    }

}
