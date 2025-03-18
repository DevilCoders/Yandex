package ru.yandex.ci.storage.core.db.model.test_revision;

import com.google.common.base.Preconditions;
import lombok.Value;

@Value
public class FragmentationSettings {
    int revisionsInRegions;
    int revisionsInBucket;

    public static FragmentationSettings create(int revisionsInBucket, int numberOfBucketsInRegion) {
        Preconditions.checkArgument(
                numberOfBucketsInRegion <= 1000,
                "Number of buckets in region can't exceed YDB fetch limit"
        );

        return new FragmentationSettings(
                revisionsInBucket * numberOfBucketsInRegion,
                revisionsInBucket
        );
    }

    public BucketNumber getBucket(long revision) {
        var region = revision / revisionsInRegions;
        var regionOffset = region * revisionsInRegions;
        var bucket = (revision - regionOffset) / revisionsInBucket;
        return new BucketNumber(region, bucket);
    }

    public long getBucketStartRevision(BucketNumber bucket) {
        return (long) bucket.getRegion() * revisionsInRegions + (long) bucket.getBucket() * revisionsInBucket;
    }

    public BucketNumber nextBucket(BucketNumber bucket) {
        if (bucket.getBucket() == revisionsInBucket - 1) {
            return new BucketNumber(bucket.getRegion() + 1, 0);
        }

        return new BucketNumber(bucket.getRegion(), bucket.getBucket() + 1);
    }
}
