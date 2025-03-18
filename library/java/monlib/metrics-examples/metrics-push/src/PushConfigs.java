package ru.yandex.monlib.metrics.example.push;

/**
 * @author Alexey Trushkin
 */
public class PushConfigs {

    public String project;
    public String cluster;
    public String service;
    public String url;
    public String token;

    public PushConfigs(String project, String cluster, String service, String url, String token) {
        this.project = project;
        this.cluster = cluster;
        this.service = service;
        this.url = url;
        this.token = token;
    }

    public String getShardString() {
        return "project='" + project + '\'' +
                ", cluster='" + cluster + '\'' +
                ", service='" + service + '\'';
    }
}
