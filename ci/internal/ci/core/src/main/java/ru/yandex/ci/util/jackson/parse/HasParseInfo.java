package ru.yandex.ci.util.jackson.parse;

public interface HasParseInfo {
    ParseInfo getParseInfo();

    void setParseInfo(ParseInfo parseInfo);
}
