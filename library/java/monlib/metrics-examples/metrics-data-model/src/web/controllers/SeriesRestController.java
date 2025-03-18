package ru.yandex.monlib.metrics.example.web.controllers;

import java.util.List;
import java.util.concurrent.CompletableFuture;

import io.swagger.annotations.Api;
import io.swagger.annotations.ApiOperation;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.monlib.metrics.example.model.dto.SeriesDto;
import ru.yandex.monlib.metrics.example.services.series.SeriesService;
import ru.yandex.monlib.metrics.example.web.controllers.validation.SeriesValidator;

/**
 * @author Alexey Trushkin
 */
@Api(tags = {"series"})
@RestController
@RequestMapping(path = "/api/v1/series", produces = MediaType.APPLICATION_JSON_VALUE)
public class SeriesRestController {

    private final SeriesService seriesService;
    private final SeriesValidator validator;

    public SeriesRestController(SeriesService seriesService, SeriesValidator validator) {
        this.seriesService = seriesService;
        this.validator = validator;
    }

    @ApiOperation(value = "Delete series with id", response = SeriesDto.class)
    @DeleteMapping("/{id}")
    public CompletableFuture<ResponseEntity<SeriesDto>> deleteSeries(@PathVariable("id") Long id) {
        return CompletableFuture.supplyAsync(() -> {
            var series = seriesService.delete(id);
            if (series == null) {
                return new ResponseEntity<>(HttpStatus.NOT_FOUND);
            } else {
                return ResponseEntity.ok(series);
            }
        });
    }

    @ApiOperation(value = "Get series by id", response = SeriesDto.class)
    @GetMapping("/{id}")
    public CompletableFuture<ResponseEntity<SeriesDto>> getSeries(@PathVariable("id") Long id) {
        return CompletableFuture.supplyAsync(() -> {
            var series = seriesService.getSeries(id);
            if (series == null) {
                return new ResponseEntity<>(HttpStatus.NOT_FOUND);
            } else {
                return ResponseEntity.ok(series);
            }
        });
    }

    @ApiOperation(value = "Get all series", response = SeriesDto.class)
    @GetMapping
    public CompletableFuture<List<SeriesDto>> getSeries() {
        return CompletableFuture.supplyAsync(seriesService::getSeries);
    }

    @ApiOperation(value = "Create series", response = SeriesDto.class)
    @PostMapping
    public CompletableFuture<ResponseEntity<Object>> createSeries(@RequestBody SeriesDto series) {
        var errors = validator.validate(series);
        if (!errors.isEmpty()) {
            return CompletableFuture.completedFuture(new ResponseEntity<>(errors, HttpStatus.BAD_REQUEST));
        }
        return CompletableFuture.supplyAsync(() -> ResponseEntity.ok(seriesService.create(series)));
    }

    @ApiOperation(value = "View episode", response = Long.class)
    @PostMapping("/episode/{id}/increment-view")
    public CompletableFuture<ResponseEntity<Long>> view(@PathVariable("id") Long id) {
        return CompletableFuture.supplyAsync(() -> {
            var count = seriesService.viewEpisode(id);
            if (count == null) {
                return new ResponseEntity<>(HttpStatus.BAD_REQUEST);
            } else {
                return ResponseEntity.ok(count);
            }
        });
    }
}
