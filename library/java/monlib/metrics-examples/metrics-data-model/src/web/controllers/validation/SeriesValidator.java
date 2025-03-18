package ru.yandex.monlib.metrics.example.web.controllers.validation;

import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import org.springframework.stereotype.Component;

import ru.yandex.monlib.metrics.example.model.dto.SeriesDto;

/**
 * @author Alexey Trushkin
 */
@Component
public class SeriesValidator {

    public static final Pattern SLUG_PATTERN = Pattern.compile("^[a-zA-Z0-9\\./@_][ 0-9a-zA-Z\\./@_,:;()\\[\\]<>-]{0,198}$");
    public static final String SLUG_ERROR_MSG = "Invalid slug for episode title = %s, see https://wiki.yandex-team.ru/solomon/userguide/metricsnamingconventions/#metki for patter";

    public List<String> validate(SeriesDto series) {
        return Optional.ofNullable(series.seasons).orElse(Collections.emptyList())
                .stream().flatMap(seasonDto -> Optional.ofNullable(seasonDto.episodes).orElse(Collections.emptyList()).stream())
                .map(episodeDto -> {
                    if (episodeDto.slug == null || !SLUG_PATTERN.matcher(episodeDto.slug).matches()) {
                        return String.format(SLUG_ERROR_MSG, episodeDto.title);
                    }
                    return null;
                })
                .filter(Objects::nonNull)
                .collect(Collectors.toList());
    }
}
