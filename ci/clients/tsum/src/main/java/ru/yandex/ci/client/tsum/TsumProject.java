package ru.yandex.ci.client.tsum;

import java.util.List;
import java.util.Map;

import lombok.Value;

@Value
public class TsumProject {
    String id;
    String title;
    List<String> releasePipelineIds;
    List<TsumDeliveryMachines> deliveryMachines;
    Map<String, TsumPipeline> pipelines;
}
