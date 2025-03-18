package ru.yandex.monlib.metrics.example.services.series;

import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics;

/**
 * @author Alexey Trushkin
 */
public class SeriesServiceTest {

    private SeriesService service;

    @Before
    public void setUp() {
        SeriesMetrics metrics = new SeriesMetrics();
        service = new SeriesService(metrics);
    }

    @Test
    public void create() {
        var seriesDto = service.create(SeriesFactory.seriesCreateDto());
        Assert.assertEquals(SeriesFactory.seriesCreatedDto(), seriesDto);
    }

    @Test
    public void createWithoutSeasons() {
        var seriesDtoOriginal = SeriesFactory.seriesCreateDto();
        seriesDtoOriginal.seasons = null;
        var seriesDto = service.create(seriesDtoOriginal);

        var createdSeries = SeriesFactory.seriesCreatedDto();
        createdSeries.seasons = List.of();
        Assert.assertEquals(createdSeries, seriesDto);
    }

    @Test
    public void getSeries() {
        var seriesDto = service.create(SeriesFactory.seriesCreateDto());

        Assert.assertNull(service.getSeries(100L));
        Assert.assertEquals(SeriesFactory.seriesCreatedDto(), service.getSeries(seriesDto.id));
        Assert.assertEquals(1, service.getSeries().size());
    }

    @Test
    public void deleteSeries() {
        var seriesDto = service.create(SeriesFactory.seriesCreateDto());

        service.delete(seriesDto.id);

        Assert.assertNull(service.getSeries(seriesDto.id));
        Assert.assertEquals(0, service.getSeries().size());
    }
}
