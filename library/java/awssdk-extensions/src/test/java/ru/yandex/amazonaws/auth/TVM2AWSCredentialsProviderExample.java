package ru.yandex.amazonaws.auth;

import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.sqs.AmazonSQS;
import com.amazonaws.services.sqs.AmazonSQSClientBuilder;
import com.amazonaws.services.sqs.model.ReceiveMessageRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.passport.tvmauth.NativeTvmClient;
import ru.yandex.passport.tvmauth.TvmApiSettings;

public final class TVM2AWSCredentialsProviderExample {
    private static final Logger LOGGER = LoggerFactory.getLogger(TVM2AWSCredentialsProviderExample.class);

    // Код сервиса SQS over KiKiMR, совместимый с Amazon SQS: https://wiki.yandex-team.ru/sqs/
    private static final int SQS_PRODUCTION_ID = 2002456;

    private TVM2AWSCredentialsProviderExample() {
        //
    }

    public static void main(String[] args) {

        final String endpoint = "http://sqs.yandex.net:8771";
        final String region = "eu-west-1";

        final String accessKey = System.getProperty("accessKey"); // Ваш аккаунт в SQS over KiKiMR
        final String secretKey = "unused";

        final int clientId = Integer.getInteger("clientId"); // ID вашего сервиса
        final String secret = System.getProperty("secret"); // Секрет вашего сервиса

        final NativeTvmClient client = new NativeTvmClient(new TvmApiSettings()
                .setSelfTvmId(clientId)
                .enableServiceTicketsFetchOptions(secret, new int[]{SQS_PRODUCTION_ID}));

        final AmazonSQS sqs = AmazonSQSClientBuilder.standard()
                .withEndpointConfiguration(new AwsClientBuilder.EndpointConfiguration(endpoint, region))
                .withCredentials(new TVM2AWSCredentialsProvider(client, accessKey, secretKey, SQS_PRODUCTION_ID))
                .build();

        final String queueName = System.getProperty("queueName"); // Название очереди
        final String queueUrl = sqs.getQueueUrl(queueName).getQueueUrl();

        while (!Thread.interrupted()) {
            LOGGER.info("Queue: {}", sqs.receiveMessage(new ReceiveMessageRequest()
                    .withQueueUrl(queueUrl)
                    .withWaitTimeSeconds(20))
                    .getMessages());
        }
    }

}
