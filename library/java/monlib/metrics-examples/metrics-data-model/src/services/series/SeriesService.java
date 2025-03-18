package ru.yandex.monlib.metrics.example.services.series;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;

import org.springframework.stereotype.Service;

import ru.yandex.monlib.metrics.example.model.dto.EpisodeDto;
import ru.yandex.monlib.metrics.example.model.dto.SeasonDto;
import ru.yandex.monlib.metrics.example.model.dto.SeriesDto;
import ru.yandex.monlib.metrics.example.model.entities.Episode;
import ru.yandex.monlib.metrics.example.model.entities.Season;
import ru.yandex.monlib.metrics.example.model.entities.Series;
import ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics;

/**
 * @author Alexey Trushkin
 */
@Service
public class SeriesService {

    // metrics
    private final SeriesMetrics seriesMetrics;

    // data
    private final ConcurrentHashMap<Long, Series> idsToSeries = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<Long, Episode> idsToEpisodes = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<Long, List<Season>> seriesToSeasons = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<Long, List<Episode>> seasonsToEpisodes = new ConcurrentHashMap<>();
    private final AtomicLong seriesIds = new AtomicLong(0L);
    private final AtomicLong seasonsIds = new AtomicLong(0L);
    private final AtomicLong episodesIds = new AtomicLong(0L);

    public SeriesService(SeriesMetrics seriesMetrics) {
        this.seriesMetrics = seriesMetrics;

        // register lazy metrics with service state
        seriesMetrics.registerType("seasons", seasonsToEpisodes::size);
        seriesMetrics.registerType("episodes", idsToEpisodes::size);
    }

    public SeriesDto create(SeriesDto seriesDto) {
        var series = Series.from(seriesIds.incrementAndGet(), seriesDto);
        idsToSeries.put(series.getId(), series);

        var seasonDtos = Optional.ofNullable(seriesDto.seasons).orElse(Collections.emptyList());
        List<Season> seasons = new ArrayList<>(seasonDtos.size());
        for (SeasonDto seasonDto : seasonDtos) {
            var season = Season.from(seasonsIds.incrementAndGet(), series.getId(), seasonDto);
            var episodeDtos = Optional.ofNullable(seasonDto.episodes).orElse(Collections.emptyList());
            var episodes = episodeDtos.stream()
                    .map(episodeDto -> Episode.from(episodesIds.incrementAndGet(), series.getId(), season.getId(), episodeDto))
                    .peek(episode -> idsToEpisodes.put(episode.getId(), episode))
                    .collect(Collectors.toList());

            seasons.add(season);
            seasonsToEpisodes.put(season.getId(), episodes);
        }
        seriesToSeasons.put(series.getId(), seasons);

        // increment eager series count metric
        seriesMetrics.incrementSeriesCount();

        return getSeries(series.getId());
    }

    public List<SeriesDto> getSeries() {
        return idsToSeries.keySet().stream()
                .map(this::getSeries)
                .collect(Collectors.toList());
    }

    public SeriesDto getSeries(long id) {
        var series = idsToSeries.get(id);
        if (series == null) {
            return null;
        }
        List<Season> seasons = seriesToSeasons.getOrDefault(series.getId(), Collections.emptyList());
        List<SeasonDto> seasonDtos = new ArrayList<>(seasons.size());
        for (Season season : seasons) {
            List<Episode> episodes = seasonsToEpisodes.getOrDefault(season.getId(), Collections.emptyList());
            List<EpisodeDto> episodeDtos = episodes.stream()
                    .map(Episode::toDto)
                    .collect(Collectors.toList());

            var seasonDto = season.toDto(episodeDtos);
            seasonDtos.add(seasonDto);
        }
        return series.toDto(seasonDtos);
    }

    public Long viewEpisode(long id) {
        var episode = idsToEpisodes.get(id);
        if (episode == null) {
            return null;
        }
        return seriesMetrics.incrementViewCount(episode.getId(), episode.getSlug());
    }

    public SeriesDto delete(long id) {
        var result = getSeries(id);
        if (result == null) {
            return null;
        }
        idsToSeries.remove(id);
        var seasons = seriesToSeasons.remove(id);
        for (var season : seasons) {
            var episodes = seasonsToEpisodes.remove(season.getId());
            for (var episode : episodes) {
                idsToEpisodes.remove(episode.getId());
                seriesMetrics.removeEpisodeMetrics(episode.getId());
            }
        }

        // decrement eager series count metric
        seriesMetrics.decrementSeriesCount();

        return result;
    }
}
