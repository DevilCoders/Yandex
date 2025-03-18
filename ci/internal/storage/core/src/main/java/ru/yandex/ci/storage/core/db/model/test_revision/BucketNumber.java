package ru.yandex.ci.storage.core.db.model.test_revision;

import lombok.AllArgsConstructor;
import lombok.Value;

@Value
@AllArgsConstructor
public class BucketNumber {
    int region;

    int bucket;

    public BucketNumber(long region, long bucket) {
        this.region = (int) region;
        this.bucket = (int) bucket;
    }

    @Override
    public String toString() {
        return "[%d/%03d]".formatted(region, bucket);
    }
}
