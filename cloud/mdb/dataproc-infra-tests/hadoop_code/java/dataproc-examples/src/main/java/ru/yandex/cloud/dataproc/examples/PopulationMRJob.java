package ru.yandex.cloud.dataproc.examples;

import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

import com.opencsv.CSVReader;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.Tool;
import org.apache.hadoop.util.ToolRunner;
import org.json.JSONObject;

public class PopulationMRJob extends Configured implements Tool {

    public static class TokenizerMapper
            extends Mapper<Object, Text, Text, IntWritable>{
        private Map<String, String> countryNameByCode = new HashMap<>();

        protected void setup(Context context) throws IOException, InterruptedException {
            byte[] encoded = Files.readAllBytes(Paths.get("config.json"));
            String jsonString = new String(encoded, Charset.defaultCharset());
            JSONObject config = new JSONObject(jsonString);

            FileReader filereader = new FileReader("country-codes.csv.zip/country-codes.csv");
            CSVReader csvReader = new CSVReader(filereader);
            String[] nextRecord;
            int lineNumber = 0;
            Map<String, Integer> columns = new HashMap<>();

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
        }

        public void map(Object key, Text value, Context context
        ) throws IOException, InterruptedException {
            String[] cells = value.toString().split("\t");
            IntWritable pop = new IntWritable(Integer.parseInt(cells[14]));
            String countryCode = cells[8];
            String countryName = countryNameByCode.getOrDefault(countryCode, countryCode);
            context.write(new Text(countryName), pop);
        }
    }

    public static class IntSumReducer
            extends Reducer<Text,IntWritable,Text,IntWritable> {
        private IntWritable result = new IntWritable();

        public void reduce(Text key, Iterable<IntWritable> values,
                           Context context
        ) throws IOException, InterruptedException {
            int sum = 0;
            for (IntWritable val : values) {
                sum += val.get();
            }
            result.set(sum);
            context.write(key, result);
        }
    }

    public static void main(String[] args) throws Exception {
        int res = ToolRunner.run(new Configuration(), new PopulationMRJob(), args);
        System.exit(res);
    }

    public int run(String[] args) throws Exception {
        Configuration conf = getConf();

        Job job = Job.getInstance(conf, "population");
        job.setJarByClass(PopulationMRJob.class);
        job.setMapperClass(TokenizerMapper.class);
        job.setCombinerClass(IntSumReducer.class);
        job.setReducerClass(IntSumReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(IntWritable.class);

        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));

        return job.waitForCompletion(true) ? 0 : 1;
    }
}
