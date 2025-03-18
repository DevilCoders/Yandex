package ru.yandex.ci.common.bazinga.spring;

import java.nio.file.Path;

import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.S3LogStorage;

@Configuration
public class S3LogStorageConfig {

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public AmazonS3 amazonS3(
            @Value("${bazinga.amazonS3.endpoint}") String endpoint,
            @Value("${bazinga.amazonS3.region}") String region,
            @Value("${bazinga.amazonS3.accessKeyId}") String accessKeyId,
            @Value("${bazinga.amazonS3.secretAccessKeyId}") String secretAccessKeyId
    ) {
        var credentials = new AWSStaticCredentialsProvider(new BasicAWSCredentials(accessKeyId, secretAccessKeyId));
        return AmazonS3ClientBuilder.standard()
                .withEndpointConfiguration(new AwsClientBuilder.EndpointConfiguration(endpoint, region))
                .withCredentials(credentials)
                .build();
    }

    @Bean
    public S3LogStorage logStorageS3(
            AmazonS3 amazonS3,
            @Value("${bazinga.logStorageS3.s3Bucket}") String s3Bucket,
            @Value("${bazinga.logStorageS3.s3Env}") String s3Env,
            @Value("${bazinga.logStorageS3.workdir}") String workdir
    ) {
        return new S3LogStorage(amazonS3, s3Bucket, s3Env, Path.of(workdir, "bazinga-logs").toString());
    }
}
