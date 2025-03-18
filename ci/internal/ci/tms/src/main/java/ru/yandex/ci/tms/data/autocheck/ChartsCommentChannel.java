package ru.yandex.ci.tms.data.autocheck;

public class ChartsCommentChannel {
    private final String feed;
    private final String color;

    public ChartsCommentChannel(String feed, String color) {
        this.feed = feed;
        this.color = color;
    }

    public String getFeed() {
        return feed;
    }

    public String getColor() {
        return color;
    }
}
