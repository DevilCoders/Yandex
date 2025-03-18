package ru.yandex.monlib.metrics.example.services.series;

import java.util.List;

import ru.yandex.monlib.metrics.example.model.dto.EpisodeDto;
import ru.yandex.monlib.metrics.example.model.dto.SeasonDto;
import ru.yandex.monlib.metrics.example.model.dto.SeriesDto;

/**
 * @author Alexey Trushkin
 */
public class SeriesFactory {

    private static final String SERIES_TITLE = "series title";
    private static final String SERIES_DESCRIPTION = "series desc";

    private static final String SEASON_TITLE = "season title";

    private static final String EPISODE_TITLE = "episode title";
    private static final String EPISODE_SLUG = "episode_slug";
    private static final Long EPISODE_DURATION = 1L;

    public static SeriesDto seriesCreateDto() {
        EpisodeDto episodeDto = new EpisodeDto();
        episodeDto.title = EPISODE_TITLE;
        episodeDto.slug = EPISODE_SLUG;
        episodeDto.duration = EPISODE_DURATION;

        SeasonDto seasonDto = new SeasonDto();
        seasonDto.title = SEASON_TITLE;
        seasonDto.episodes = List.of(episodeDto);

        SeriesDto seriesDto = new SeriesDto();
        seriesDto.title = SERIES_TITLE;
        seriesDto.description = SERIES_DESCRIPTION;
        seriesDto.seasons = List.of(seasonDto);

        return seriesDto;
    }

    public static SeriesDto seriesCreatedDto() {
        EpisodeDto episodeDto = new EpisodeDto();
        episodeDto.title = EPISODE_TITLE;
        episodeDto.slug = EPISODE_SLUG;
        episodeDto.duration = EPISODE_DURATION;
        episodeDto.id = 1L;

        SeasonDto seasonDto = new SeasonDto();
        seasonDto.title = SEASON_TITLE;
        seasonDto.id = 1L;
        seasonDto.episodes = List.of(episodeDto);

        SeriesDto seriesDto = new SeriesDto();
        seriesDto.title = SERIES_TITLE;
        seriesDto.description = SERIES_DESCRIPTION;
        seriesDto.id = 1L;
        seriesDto.seasons = List.of(seasonDto);

        return seriesDto;
    }
}
