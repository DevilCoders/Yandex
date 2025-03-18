package ru.yandex.monlib.metrics.example.model.dto;

import java.util.List;
import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonInclude;
import io.swagger.annotations.ApiModel;
import io.swagger.annotations.ApiModelProperty;

/**
 * @author Alexey Trushkin
 */
@ApiModel("Series")
@JsonInclude(JsonInclude.Include.NON_NULL)
public class SeriesDto {

    @ApiModelProperty(
            value = "Series id")
    public Long id;

    @ApiModelProperty(
            value = "Series title",
            required = true)
    public String title;

    @ApiModelProperty(
            value = "Series description",
            required = true)
    public String description;

    @ApiModelProperty(
            value = "Series seasons",
            required = true)
    public List<SeasonDto> seasons;

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        SeriesDto seriesDto = (SeriesDto) o;
        return Objects.equals(id, seriesDto.id) && Objects.equals(title, seriesDto.title) && Objects.equals(description, seriesDto.description) && Objects.equals(seasons, seriesDto.seasons);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, title, description, seasons);
    }
}
