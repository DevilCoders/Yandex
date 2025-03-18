package ru.yandex.monlib.metrics.example.push.services;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.fasterxml.jackson.databind.ObjectMapper;

import ru.yandex.monlib.metrics.example.model.dto.EpisodeDto;
import ru.yandex.monlib.metrics.example.model.dto.SeriesDto;
import ru.yandex.monlib.metrics.example.services.series.SeriesService;
import ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics;

/**
 * Imitate background job working with series: create + view-increment
 * <p>
 * Run several tasks for series metrics incrementing
 *
 * @author Alexey Trushkin
 */
public class SeriesBotService {

    private static final int MAX_INCREMENT_TIMEOUT_MS = 50;
    private static final int CREATE_SERIES_TIMEOUT_MS = 10_000;

    private final SeriesService seriesService;
    private final ObjectMapper objectMapper;
    private final String json;
    private final ExecutorService executorService;

    public SeriesBotService(SeriesMetrics seriesMetrics, String jsonPath) throws IOException {
        seriesService = new SeriesService(seriesMetrics);
        objectMapper = new ObjectMapper();
        json = new String(Files.readAllBytes(Paths.get(jsonPath)));
        executorService = Executors.newCachedThreadPool(new InnerThreadFactory());
    }

    public void start() throws IOException {
        var type = objectMapper.getTypeFactory().constructCollectionType(List.class, SeriesDto.class);
        List<SeriesDto> seriesDtos = objectMapper.readValue(json, type);
        List<SeriesDto> createdSeries = new ArrayList<>(seriesDtos.size());
        for (var series : seriesDtos) {
            createdSeries.add(seriesService.create(series));
        }

        var episodes = createdSeries.stream()
                .flatMap(seriesDto -> seriesDto.seasons.stream())
                .flatMap(seasonDto -> seasonDto.episodes.stream())
                .collect(Collectors.toList());

        // run episode view metric increment
        episodes.forEach(this::runEpisodeViewer);
        // create more series
        runSeriesCreator(seriesDtos);
    }

    private void runSeriesCreator(List<SeriesDto> seriesDtos) {
        executorService.execute(() -> {
            while (true) {
                for (var series : seriesDtos) {
                    seriesService.create(series);
                }
                try {
                    TimeUnit.MILLISECONDS.sleep(CREATE_SERIES_TIMEOUT_MS);
                } catch (InterruptedException ignore) {
                }
            }
        });
    }

    private void runEpisodeViewer(EpisodeDto episodeDto) {
        executorService.execute(() -> {
            var rnd = ThreadLocalRandom.current();
            while (true) {
                seriesService.viewEpisode(episodeDto.id);
                try {
                    TimeUnit.MILLISECONDS.sleep(rnd.nextInt(MAX_INCREMENT_TIMEOUT_MS));
                } catch (InterruptedException ignore) {
                }
            }
        });
    }

    private static class InnerThreadFactory implements ThreadFactory {
        int count = 0;

        @Override
        public Thread newThread(Runnable r) {
            Thread thread = new Thread(r);
            thread.setDaemon(true);
            thread.setName(SeriesBotService.class.getSimpleName() + "-executor-" + (count++));
            return thread;
        }
    }
}
