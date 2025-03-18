package ru.yandex.ci.tms.task.autocheck.degradation;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.charts.ChartsClient;
import ru.yandex.ci.client.charts.model.ChartsCommentRequest;
import ru.yandex.ci.client.charts.model.ChartsCommentType;
import ru.yandex.ci.client.charts.model.ChartsGetCommentRequest;
import ru.yandex.ci.client.charts.model.ChartsGetCommentResponse;
import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.model.FilterRequest;
import ru.yandex.ci.client.juggler.model.MuteCreateRequest;
import ru.yandex.ci.client.juggler.model.MuteFilterRequest;
import ru.yandex.ci.client.juggler.model.MuteInfoResponse;
import ru.yandex.ci.client.juggler.model.MuteRemoveRequest;
import ru.yandex.ci.client.juggler.model.MutesRequest;
import ru.yandex.ci.tms.data.autocheck.ChartsCommentChannel;

public class AutocheckDegradationNotificationsManager {
    private static final Logger log = LoggerFactory.getLogger(AutocheckDegradationNotificationsManager.class);

    private final int muteDurationMinutes;
    private final int muteOverlayMinutes;
    private final String robotLogin;
    private final MutesRequest mutesRequest;
    private final List<FilterRequest> muteFilters;
    private final List<ChartsCommentChannel> commentsChannels;

    //Clients
    private final JugglerClient jugglerClient;
    private final ChartsClient chartsClient;

    public AutocheckDegradationNotificationsManager(
            int muteDurationMinutes,
            int muteOverlayMinutes,
            String robotLogin,
            List<MuteFilterRequest> muteFilters,
            List<ChartsCommentChannel> commentsChannels,
            JugglerClient jugglerClient,
            ChartsClient chartsClient
    ) {
        this.muteDurationMinutes = muteDurationMinutes;
        this.muteOverlayMinutes = muteOverlayMinutes;
        this.robotLogin = robotLogin;
        this.mutesRequest = new MutesRequest(muteFilters, 0, 1000, false);
        this.muteFilters = muteFilters.stream().map(m -> new FilterRequest(
                m.getHost(), m.getNamespace(), m.getService(), m.getTags()
        )).collect(Collectors.toList());
        this.commentsChannels = List.copyOf(commentsChannels);

        this.jugglerClient = jugglerClient;
        this.chartsClient = chartsClient;
    }

    public void muteJuggler() {
        var mutes = jugglerClient.getMutes(mutesRequest);
        if (
                mutes.isEmpty() || mutes.stream()
                        .allMatch(
                                m -> m.getEndTime().isBefore(Instant.now().plus(muteOverlayMinutes, ChronoUnit.MINUTES))
                        )
        ) {
            var request = new MuteCreateRequest(
                    "muted automatically",
                    Instant.now(),
                    Instant.now().plus(muteDurationMinutes, ChronoUnit.MINUTES),
                    muteFilters
            );

            jugglerClient.createMute(request);
        }
    }

    public void unmuteJuggler() {
        List<String> muteIds = jugglerClient.getMutes(mutesRequest).stream()
                .map(MuteInfoResponse::getId).collect(Collectors.toList());

        jugglerClient.removeMutes(new MuteRemoveRequest(muteIds));
    }

    public Optional<RuntimeException> openChartsCommentsAboutBlockedDegradation(String text) {
        try {
            for (ChartsCommentChannel channel : commentsChannels) {
                if (getComments(channel).findAny().isEmpty()) {
                    chartsClient.createComment(new ChartsCommentRequest(
                            channel.getFeed(),
                            ChartsCommentType.REGION,
                            Instant.now(),
                            Instant.now().plus(1, ChronoUnit.DAYS),
                            text,
                            Map.of("visible", true, "color", channel.getColor())
                    ));
                }
            }
            return Optional.empty();
        } catch (RuntimeException ex) {
            log.error("Unable to open Charts comments", ex);
            return Optional.of(ex);
        }
    }

    public Optional<RuntimeException> closeChartsCommentsAboutBlockedDegradation() {
        try {
            for (ChartsCommentChannel channel : commentsChannels) {
                getComments(channel).forEach(c ->
                        chartsClient.updateComment(
                                c.getId(),
                                new ChartsCommentRequest(channel.getFeed(), null, null, Instant.now(), null, null)
                        ));
            }
            return Optional.empty();
        } catch (RuntimeException ex) {
            log.error("Unable to close Charts comments", ex);
            return Optional.of(ex);
        }
    }

    private Stream<ChartsGetCommentResponse> getComments(ChartsCommentChannel channel) {
        return chartsClient.getComments(
                new ChartsGetCommentRequest(
                        channel.getFeed(),
                        Instant.now(),
                        Instant.now().plus(2, ChronoUnit.DAYS)
                )
        ).stream().filter(c -> robotLogin.equals(c.getCreatorLogin()));
    }
}
