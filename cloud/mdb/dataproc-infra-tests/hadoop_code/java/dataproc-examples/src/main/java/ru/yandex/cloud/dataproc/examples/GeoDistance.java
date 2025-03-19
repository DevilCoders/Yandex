package ru.yandex.cloud.dataproc.examples;

import java.lang.Math;
import org.apache.hadoop.hive.ql.exec.UDF;

public final class GeoDistance extends UDF
{
    public double evaluate(double lat1, double lon1, double lat2, double lon2) {
        double latDistance = Math.toRadians(lat2 - lat1);
        double lonDistance = Math.toRadians(lon2 - lon1);
        double a = Math.sin(latDistance / 2) * Math.sin(latDistance / 2)
                + Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2))
                * Math.sin(lonDistance / 2) * Math.sin(lonDistance / 2);
        double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

        final int R = 6371; // Radius of the earth
        return R * c;
    }
}
