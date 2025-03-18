package ru.yandex.monlib.metrics.example.model.dto;

import java.util.List;
import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonInclude;
import io.swagger.annotations.ApiModel;
import io.swagger.annotations.ApiModelProperty;

/**
 * @author Alexey Trushkin
 */
@ApiModel("Season")
@JsonInclude(JsonInclude.Include.NON_NULL)
public class SeasonDto {

    @ApiModelProperty(
            value = "Season title",
            required = true)
    public String title;

    @ApiModelProperty(
            value = "Season id")
    public Long id;

    @ApiModelProperty(
            value = "Season episodes",
            required = true)
    public List<EpisodeDto> episodes;

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        SeasonDto seasonDto = (SeasonDto) o;
        return Objects.equals(title, seasonDto.title) && Objects.equals(id, seasonDto.id) && Objects.equals(episodes, seasonDto.episodes);
    }

    @Override
    public int hashCode() {
        return Objects.hash(title, id, episodes);
    }
}
