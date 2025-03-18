package ru.yandex.monlib.metrics.example.model.entities;

import java.util.Objects;

import ru.yandex.monlib.metrics.example.model.dto.EpisodeDto;

/**
 * @author Alexey Trushkin
 */
public class Episode {

    private final long seriesId;
    private final long seasonId;
    private final long id;
    private final String title;
    private final String slug;
    private final long duration;

    public Episode(long id, long seriesId, long seasonId, String title, String slug, long duration) {
        this.seriesId = seriesId;
        this.seasonId = seasonId;
        this.id = id;
        this.title = title;
        this.duration = duration;
        this.slug = slug;
    }

    public static Episode from(long id, long seriesId, long seasonId, EpisodeDto episodeDto) {
        return new Episode(id, seriesId, seasonId, episodeDto.title, episodeDto.slug, episodeDto.duration);
    }

    public EpisodeDto toDto() {
        var episodeDto = new EpisodeDto();
        episodeDto.duration = duration;
        episodeDto.title = title;
        episodeDto.id = id;
        episodeDto.slug = slug;
        return episodeDto;
    }

    public long getSeriesId() {
        return seriesId;
    }

    public long getSeasonId() {
        return seasonId;
    }

    public long getId() {
        return id;
    }

    public String getTitle() {
        return title;
    }

    public long getDuration() {
        return duration;
    }

    public String getSlug() {
        return slug;
    }

    @Override
    public String toString() {
        return "Episode{" +
                "seriesId=" + seriesId +
                ", seasonId=" + seasonId +
                ", id=" + id +
                ", title='" + title + '\'' +
                ", slug='" + slug + '\'' +
                ", duration=" + duration +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Episode episode = (Episode) o;
        return seriesId == episode.seriesId && seasonId == episode.seasonId && id == episode.id && duration == episode.duration && Objects.equals(title, episode.title) && Objects.equals(slug, episode.slug);
    }

    @Override
    public int hashCode() {
        return Objects.hash(seriesId, seasonId, id, title, slug, duration);
    }
}
