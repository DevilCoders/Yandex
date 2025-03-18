package ru.yandex.ci.observer.api.controllers;

import java.util.List;
import java.util.stream.Collectors;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import lombok.Value;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.ci.observer.api.statistics.aggregated.AggregatedStatisticsCollector;
import ru.yandex.ci.observer.api.statistics.model.Sensor;
import ru.yandex.ci.observer.api.statistics.model.StatisticsItem;

@RestController
@RequestMapping(
        method = RequestMethod.GET,
        path = "/api/solomon"
)
public class SolomonApiController {
    private final AggregatedStatisticsCollector aggregatedStatsCollector;
    private final Gson jsonSerializer = new GsonBuilder().create();

    @Autowired
    public SolomonApiController(AggregatedStatisticsCollector aggregatedStatsCollector) {
        this.aggregatedStatsCollector = aggregatedStatsCollector;
    }

    @GetMapping(path = "/aggregated/main")
    public String getMainStatistics() {
        return jsonSerializer.toJson(
                new SensorsResponse(
                        aggregatedStatsCollector.getMainStatisticsCache().stream()
                                .map(StatisticsItem::toSensor)
                                .collect(Collectors.toList())
                )
        );
    }

    @GetMapping(path = "/aggregated/additional/other")
    public String getAdditionalStatistics() {
        return jsonSerializer.toJson(
                new SensorsResponse(
                        aggregatedStatsCollector.getAdditionalStatisticsCache().stream()
                                .map(StatisticsItem::toSensor)
                                .collect(Collectors.toList())
                )
        );
    }

    @GetMapping(path = "/aggregated/additional/percentiles")
    public String getAdditionalPercentilesStatistics() {
        return jsonSerializer.toJson(
                new SensorsResponse(
                        aggregatedStatsCollector.getAdditionalPercentileStatisticsCache().stream()
                                .map(StatisticsItem::toSensor)
                                .collect(Collectors.toList())
                )
        );
    }

    @Value
    public static class SensorsResponse {
        List<Sensor> metrics;
    }
}
