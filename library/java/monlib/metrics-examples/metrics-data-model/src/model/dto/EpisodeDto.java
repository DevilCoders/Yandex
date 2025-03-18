package ru.yandex.monlib.metrics.example.model.dto;

import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonInclude;
import io.swagger.annotations.ApiModel;
import io.swagger.annotations.ApiModelProperty;

/**
 * @author Alexey Trushkin
 */
@ApiModel("Episode")
@JsonInclude(JsonInclude.Include.NON_NULL)
public class EpisodeDto {

    @ApiModelProperty(
            value = "Episode id")
    public Long id;

    @ApiModelProperty(
            value = "Episode title",
            required = true)
    public String title;

    @ApiModelProperty(
            value = "Episode slug",
            required = true)
    public String slug;

    @ApiModelProperty(
            value = "Episode duration in seconds",
            required = true)
    public Long duration;

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        EpisodeDto that = (EpisodeDto) o;
        return Objects.equals(id, that.id) && Objects.equals(title, that.title) && Objects.equals(slug, that.slug) && Objects.equals(duration, that.duration);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, title, slug, duration);
    }
}
