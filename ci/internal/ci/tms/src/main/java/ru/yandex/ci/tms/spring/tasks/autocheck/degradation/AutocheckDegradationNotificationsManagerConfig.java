package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import java.util.List;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.charts.ChartsClient;
import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.model.MuteFilterRequest;
import ru.yandex.ci.tms.data.autocheck.ChartsCommentChannel;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationNotificationsManager;

@Configuration
@Import({
        JugglerClientConfig.class,
        ChartsClientConfig.class
})
public class AutocheckDegradationNotificationsManagerConfig {
    @Bean
    public List<ChartsCommentChannel> commentsChannels(
            @Value("${ci.commentsChannels.feed}") String feed,
            @Value("${ci.commentsChannels.color}") String color
    ) {
        return List.of(new ChartsCommentChannel(feed, color));
    }

    @Bean
    public List<MuteFilterRequest> muteFilters(
            @Value("${ci.muteFilters.filterNamespace}") String filterNamespace,
            @Value("${ci.muteFilters.filterTags}") String[] filterTags,
            @Value("${ci.muteFilters.robotLogin}") String robotLogin
    ) {
        return List.of(new MuteFilterRequest(null, filterNamespace, null, List.of(filterTags), robotLogin));
    }

    @Bean
    public AutocheckDegradationNotificationsManager notificationsManager(
            @Value("${ci.notificationsManager.durationMinutes}") int durationMinutes,
            @Value("${ci.notificationsManager.overlayMinutes}") int overlayMinutes,
            @Value("${ci.notificationsManager.robotLogin}") String robotLogin,
            List<MuteFilterRequest> muteFilters,
            List<ChartsCommentChannel> commentsChannels,
            JugglerClient jugglerClient,
            ChartsClient chartsClient
    ) {
        return new AutocheckDegradationNotificationsManager(
                durationMinutes,
                overlayMinutes,
                robotLogin,
                muteFilters,
                commentsChannels,
                jugglerClient,
                chartsClient
        );
    }
}
