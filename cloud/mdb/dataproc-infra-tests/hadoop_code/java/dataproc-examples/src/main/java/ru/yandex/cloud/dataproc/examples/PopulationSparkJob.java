package ru.yandex.cloud.dataproc.examples;

import java.io.FileReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;
import scala.Tuple2;

import com.opencsv.CSVReader;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.sql.SparkSession;
import org.json.JSONObject;


public class PopulationSparkJob {
    public static class City {
        String country_code;
        int population;

        City(String line) {
            String[] cells = line.split("\t");
            this.country_code = cells[8];
            this.population = Integer.parseInt(cells[14]);
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("Usage: PopulationSparkJob <input-file> <output-path>");
            System.exit(1);
        }

        byte[] encoded = Files.readAllBytes(Paths.get("config.json"));
        String jsonString = new String(encoded, Charset.defaultCharset());
        JSONObject config = new JSONObject(jsonString);

        FileReader filereader = new FileReader("country-codes.csv.zip/country-codes.csv");
        CSVReader csvReader = new CSVReader(filereader);
        String[] nextRecord;
        int lineNumber = 0;
        Map<String, Integer> columns = new HashMap<>();
        Map<String, String> countryNameByCode = new HashMap<>();

        while ((nextRecord = csvReader.readNext()) != null) {
            if (lineNumber == 0) {
                for(int i = 0; i < nextRecord.length; i++) {
                    columns.put(nextRecord[i], i);
                }
            } else {
                int nameColumnIndex = columns.getOrDefault(config.getString("name_column"), 2);
                String name = nextRecord[nameColumnIndex];
                String code2 = nextRecord[6];
                name = Utils.translit(config.getString("transliterator"), name);
                countryNameByCode.put(code2, name);
            }
            lineNumber++;
        }

        SparkSession spark = SparkSession
                .builder()
                .appName("Population")
                .getOrCreate();

        String jobId = spark.conf().get("spark.yarn.tags").replace("dataproc_job_", "");
        String outputUri = args[1].replace("${JOB_ID}", jobId);

        JavaRDD<City> parsedData = spark.read().textFile(args[0])
                .javaRDD()
                .map(City::new)
                .cache();

        parsedData
                .mapToPair(city -> new Tuple2<>(city.country_code, city))
                .mapValues(city -> city.population)
                .reduceByKey(Integer::sum)
                .repartition(1)
                .map(tuple -> new Tuple2<>(countryNameByCode.getOrDefault(tuple._1, tuple._1), tuple._2))
                .map(tuple -> String.join(",", Arrays.asList(tuple._1, Integer.toString(tuple._2))))
                .saveAsTextFile(outputUri);

        spark.stop();
    }
}
