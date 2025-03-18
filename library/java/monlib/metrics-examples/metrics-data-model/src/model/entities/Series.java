package ru.yandex.monlib.metrics.example.model.entities;

import java.util.List;
import java.util.Objects;

import ru.yandex.monlib.metrics.example.model.dto.SeasonDto;
import ru.yandex.monlib.metrics.example.model.dto.SeriesDto;

/**
 * @author Alexey Trushkin
 */
public class Series {

    private final long id;
    private final String title;
    private final String description;

    public Series(long id, String title, String description) {
        this.id = id;
        this.title = title;
        this.description = description;
    }

    public static Series from(long id, SeriesDto dto) {
        return new Series(id, dto.title, dto.description);
    }

    public SeriesDto toDto(List<SeasonDto> seasonDtos) {
        var seriesDto = new SeriesDto();
        seriesDto.seasons = seasonDtos;
        seriesDto.id = id;
        seriesDto.description = description;
        seriesDto.title = title;

        return seriesDto;
    }

    public long getId() {
        return id;
    }

    public String getTitle() {
        return title;
    }

    public String getDescription() {
        return description;
    }

    @Override
    public String toString() {
        return "Series{" +
                "id=" + id +
                ", title='" + title + '\'' +
                ", description='" + description + '\'' +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Series series = (Series) o;
        return id == series.id && Objects.equals(title, series.title) && Objects.equals(description, series.description);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, title, description);
    }
}
