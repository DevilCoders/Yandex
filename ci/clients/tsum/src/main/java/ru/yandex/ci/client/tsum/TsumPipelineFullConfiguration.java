package ru.yandex.ci.client.tsum;

import java.util.List;

import lombok.Value;

@Value
public class TsumPipelineFullConfiguration {
    String id;
    List<TsumJob> jobs;
}
