package ru.yandex.monlib.metrics.example.model.entities;

import java.util.List;
import java.util.Objects;

import ru.yandex.monlib.metrics.example.model.dto.EpisodeDto;
import ru.yandex.monlib.metrics.example.model.dto.SeasonDto;

/**
 * @author Alexey Trushkin
 */
public class Season {

    private final long seriesId;
    private final long id;
    private final String title;

    public Season(long id, long seriesId, String title) {
        this.seriesId = seriesId;
        this.id = id;
        this.title = title;
    }

    public static Season from(long id, long seriesId, SeasonDto seasonDto) {
        return new Season(id, seriesId, seasonDto.title);
    }

    public SeasonDto toDto(List<EpisodeDto> episodeDtos) {
        var seasonDto = new SeasonDto();
        seasonDto.id = id;
        seasonDto.title = title;
        seasonDto.episodes = episodeDtos;
        return seasonDto;
    }

    public long getSeriesId() {
        return seriesId;
    }

    public long getId() {
        return id;
    }

    public String getTitle() {
        return title;
    }

    @Override
    public String toString() {
        return "Season{" +
                "seriesId=" + seriesId +
                ", id=" + id +
                ", title='" + title + '\'' +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Season season = (Season) o;
        return seriesId == season.seriesId && id == season.id && Objects.equals(title, season.title);
    }

    @Override
    public int hashCode() {
        return Objects.hash(seriesId, id, title);
    }

}
