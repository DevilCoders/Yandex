package ru.yandex.monlib.metrics.encode.prometheus;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * See <a href="https://github.com/prometheus/docs/blob/master/content/docs/instrumenting/exposition_formats.md">Prometheus
 * exposition formats</a> for more details.
 *
 * @author Sergey Polovko
 */
final class PrometheusParserImpl {

    // keywords
    private static final byte[] TYPE = { 'T', 'Y', 'P', 'E' };
    private static final byte[] GAUGE = { 'G', 'A', 'U', 'G', 'E' };
    private static final byte[] COUNTER = { 'C', 'O', 'U', 'N', 'T', 'E', 'R' };
    private static final byte[] SUMMARY = { 'S', 'U', 'M', 'M', 'A', 'R', 'Y' };
    private static final byte[] HISTOGRAM = { 'H', 'I', 'S', 'T', 'O', 'G', 'R', 'A', 'M' };
    private static final byte[] UNTYPED = { 'U', 'N', 'T', 'Y', 'P', 'E', 'D' };

    enum MetricType {
        GAUGE,
        COUNTER,
        SUMMARY,
        UNTYPED,
        HISTOGRAM,
    }

    private final ByteBuffer buffer;
    private final MetricConsumer consumer;
    private final Map<String, MetricType> seenTypes = new HashMap<>();

    private int currentLine = 1;
    private byte currentByte = 0;

    private byte[] tmpBuf = new byte[128];
    private HashMap<String, String> labels = new HashMap<>(Labels.MAX_LABELS_COUNT);
    private HistogramBuilder histogramBuilder = new HistogramBuilder();


    PrometheusParserImpl(ByteBuffer buffer, MetricConsumer consumer) {
        this.buffer = buffer;
        this.consumer = consumer;
    }

    void parse() {
        consumer.onStreamBegin(-1);

        if (buffer.hasRemaining()) {
            readNextByte();
            skipSpaces();

            try {
                while (buffer.hasRemaining()) {
                    switch (currentByte) {
                        case '\n':
                            readNextByte(); // skip '\n'
                            currentLine++;
                            skipSpaces();
                            break;
                        case '#':
                            parseComment();
                            break;
                        default:
                            parseMetric();
                            break;
                    }
                }

                if (!histogramBuilder.isEmpty()) {
                    consumeHistogram();
                }
            } catch (IllegalStateException e) {
                throw newParseException(e.getMessage());
            }
        }

        consumer.onStreamEnd();
    }

    // # 'TYPE' metric_name {counter|gauge|histogram|summary|untyped}
    // # 'HELP' metric_name some help info
    // # general comment message
    private void parseComment() {
        skipExpectedChar('#');
        skipSpaces();

        if (isKeyword(TYPE)) {
            skipSpaces();

            String nextName = readTokenAsMetricName();
            if (nextName.isEmpty()) {
                throw newParseException("invalid metric name");
            }

            skipSpaces();
            MetricType nextType = readType();

            MetricType prevType = seenTypes.put(nextName, nextType);
            if (prevType != null) {
                throw newParseException("second TYPE line for metric " + nextName);
            }

            if (nextType == PrometheusParserImpl.MetricType.HISTOGRAM) {
                if (!histogramBuilder.isEmpty()) {
                    consumeHistogram();
                }
                histogramBuilder.setName(nextName);
            }
        } else {
            // skip HELP and general comments
            skipUntilEol();
        }

        if (currentByte != '\n') {
            throw newParseException("expected '\\n', found " + (char) currentByte);
        }
    }

    // metric_name [labels] value [timestamp]
    private void parseMetric() {
        String name = readTokenAsMetricName();
        skipSpaces();

        Map<String, String> labels = readLabels();
        skipSpaces();

        double value = Doubles.fromString(readToken());
        skipSpaces();

        long tsMillis = 0;
        if (currentByte != '\n') {
            tsMillis = Long.parseLong(readToken());
        }

        String baseName = name;
        MetricType type = seenTypes.get(name);
        if (type == null) {
            baseName = PrometheusModel.toBaseName(name);
            type = seenTypes.getOrDefault(baseName, PrometheusParserImpl.MetricType.UNTYPED);
        }

        switch (type) {
            case HISTOGRAM:
                if (PrometheusModel.isBucket(name)) {
                    String boundStr = labels.remove(PrometheusModel.BUCKET_LABEL);
                    if (boundStr == null) {
                        throw newParseException("metric " + name + " has no '" + PrometheusModel.BUCKET_LABEL + "' label");
                    }
                    double bound = Doubles.fromString(boundStr);

                    if (!histogramBuilder.isEmpty() && !histogramBuilder.isSame(baseName, labels)) {
                        consumeHistogram();
                        histogramBuilder.setName(baseName);
                    }

                    histogramBuilder.addBucket(bound, (long) value);
                    histogramBuilder.setTsMillis(tsMillis);
                    histogramBuilder.setLabels(labels.isEmpty()
                        ? Collections.emptyMap()
                        : new HashMap<>(labels));
                } else if (PrometheusModel.isCount(name)) {
                    // translate x_count metric as COUNTER
                    consumeCounter(name, labels, tsMillis, (long) value);
                } else if (PrometheusModel.isSum(name)) {
                    // translate x_sum metric as GAUGE
                    consumeGauge(name, labels, tsMillis, value);
                } else {
                    throw newParseException("metric " + name + " should be part of HISTOGRAM " + baseName);
                }
                break;

            case SUMMARY:
                if (PrometheusModel.isCount(name)) {
                    // translate x_count metric as COUNTER
                    consumeCounter(name, labels, tsMillis, (long) value);
                } else if (PrometheusModel.isSum(name)) {
                    // translate x_sum metric as GAUGE
                    consumeGauge(name, labels, tsMillis, value);
                } else {
                    consumeGauge(name, labels, tsMillis, value);
                }
                break;

            case COUNTER:
                consumeCounter(name, labels, tsMillis, (long) value);
                break;

            case GAUGE:
                consumeGauge(name, labels, tsMillis, value);
                break;

            case UNTYPED:
                consumeGauge(name, labels, tsMillis, value);
                break;
        }

        if (currentByte != '\n') {
            throw newParseException("expected '\\n', found " + (char) currentByte);
        }
    }

    // { name = "value", name2 = "value2", }
    private Map<String, String> readLabels() {
        if (currentByte != '{') {
            return Collections.emptyMap();
        }

        skipExpectedChar('{');
        skipSpaces();

        Map<String, String> labels = Collections.emptyMap();
        while (currentByte != '}') {
            String name = readTokenAsLabelName();
            skipSpaces();

            skipExpectedChar('=');
            skipSpaces();

            String value = readTokenAsLabelValue();
            skipSpaces();

            if (labels.isEmpty()) {
                labels = this.labels;
                labels.clear();
            }
            labels.put(name, value);

            if (currentByte == ',') {
                skipExpectedChar(',');
                skipSpaces();
            }
        }

        skipExpectedChar('}');
        return labels;
    }

    private MetricType readType() {
        final int pos = buffer.position();
        switch (Ascii.toUpper(currentByte)) {
            case 'G':
                if (isKeywordIgnoreCase(GAUGE)) {
                    return PrometheusParserImpl.MetricType.GAUGE;
                }
                break;
            case 'C':
                if (isKeywordIgnoreCase(COUNTER)) {
                    return PrometheusParserImpl.MetricType.COUNTER;
                }
                break;
            case 'S':
                if (isKeywordIgnoreCase(SUMMARY)) {
                    return PrometheusParserImpl.MetricType.SUMMARY;
                }
                break;
            case 'H':
                if (isKeywordIgnoreCase(HISTOGRAM)) {
                    return PrometheusParserImpl.MetricType.HISTOGRAM;
                }
                break;
            case 'U':
                if (isKeywordIgnoreCase(UNTYPED)) {
                    return PrometheusParserImpl.MetricType.UNTYPED;
                }
                break;
        }

        buffer.position(pos);
        throw newParseException("unknown type: " + readToken());
    }

    private boolean isKeyword(byte[] keyword) {
        for (int i = 0; i < keyword.length && buffer.hasRemaining(); i++) {
            if (keyword[i] != currentByte) {
                return false;
            }
            currentByte = buffer.get();
        }
        return Ascii.isSpace(currentByte) || currentByte == '\n';
    }

    private boolean isKeywordIgnoreCase(byte[] keyword) {
        for (int i = 0; i < keyword.length && buffer.hasRemaining(); i++) {
            if (keyword[i] != Ascii.toUpper(currentByte)) {
                return false;
            }
            currentByte = buffer.get();
        }
        return Ascii.isSpace(currentByte) || currentByte == '\n';
    }

    private void readNextByte() {
        if (!buffer.hasRemaining()) {
            throw newParseException("unexpected end of file");
        }
        currentByte = buffer.get();
    }

    private void skipExpectedChar(char ch) {
        final char currentCh = (char) currentByte;
        if (currentCh != ch) {
            throw newParseException(String.format("expected '%c', found '%c'", ch, currentCh));
        }
        readNextByte();
    }

    private void skipSpaces() {
        while (buffer.hasRemaining() && Ascii.isSpace(currentByte)) {
            readNextByte();
        }
    }

    private void skipUntilEol() {
        while (buffer.hasRemaining() && currentByte != '\n') {
            readNextByte();
        }
    }

    private String readToken() {
        final int begin = Math.max(0, buffer.position() - 1); // read currentByte again
        while (buffer.hasRemaining() && !Ascii.isSpace(currentByte) && currentByte != '\n') {
            currentByte = buffer.get();
        }
        return tokenFromPos(begin);
    }

    private String readTokenAsMetricName() {
        if (!PrometheusModel.isValidMetricNameStart(currentByte)) {
            return "";
        }

        final int begin = Math.max(0, buffer.position() - 1); // read currentByte again
        while (buffer.hasRemaining()) {
            currentByte = buffer.get();
            if (!PrometheusModel.isValidMetricNameContinuation(currentByte)) {
                break;
            }
        }
        return tokenFromPos(begin);
    }

    private String readTokenAsLabelName() {
        if (!PrometheusModel.isValidLabelNameStart(currentByte)) {
            return "";
        }

        final int begin = Math.max(0, buffer.position() - 1); // read currentByte again
        while (buffer.hasRemaining()) {
            currentByte = buffer.get();
            if (!PrometheusModel.isValidLabelNameContinuation(currentByte)) {
                break;
            }
        }
        return tokenFromPos(begin);
    }

    private String readTokenAsLabelValue() {
        if (currentByte != '"') {
            throw newParseException("expected '\"' before labels value, got: " + (char) currentByte);
        }

        int count = 0;

        while (true) {
            if (count >= tmpBuf.length) {
                final int newSize = tmpBuf.length + (tmpBuf.length >>> 1); // x1.5
                tmpBuf = Arrays.copyOf(tmpBuf, newSize);
            }

            readNextByte();

            switch (currentByte) {
                case '"':
                    skipExpectedChar('"');
                    return new String(tmpBuf, 0, count, StandardCharsets.UTF_8);

                case '\n':
                    throw newParseException("label value contains unescaped new-line");

                case '\\':
                    readNextByte();
                    switch (currentByte) {
                        case '"':
                        case '\\':
                            tmpBuf[count++] = currentByte;
                            break;
                        case 'n':
                            tmpBuf[count++] = '\n';
                            break;
                        default:
                            throw newParseException(String.format("invalid escape sequence '\\%c'", currentByte));
                    }
                    break;

                default:
                    tmpBuf[count++] = currentByte;
                    break;
            }
        }
    }

    private String tokenFromPos(int begin) {
        final int len = buffer.position() - begin - 1;
        if (len == 0) {
            return "";
        }

        if (len > tmpBuf.length) {
            tmpBuf = new byte[len];
        }

        buffer.position(begin);
        buffer.get(tmpBuf, 0, len);
        buffer.get(); // skip already read currentByte
        return new String(tmpBuf, 0, len, StandardCharsets.UTF_8);
    }

    private RuntimeException newParseException(String message) {
        return new RuntimeException(message + " at line #" + currentLine);
    }

    private void consumeLabels(String name, Map<String, String> labels) {
        if (labels.containsKey("sensor")) {
            throw newParseException("label name 'sensor' is reserved, but used with metric: " + name);
        }

        consumer.onLabelsBegin(labels.size() + 1);
        consumer.onLabel("sensor", name);
        if (!labels.isEmpty()) {
            for (Map.Entry<String, String> e : labels.entrySet()) {
                consumer.onLabel(e.getKey(), e.getValue());
            }
        }
        consumer.onLabelsEnd();
    }

    private void consumeCounter(String name, Map<String, String> labels, long tsMillis, long value) {
        consumer.onMetricBegin(ru.yandex.monlib.metrics.MetricType.COUNTER);
        consumeLabels(name, labels);
        consumer.onLong(tsMillis, value);
        consumer.onMetricEnd();
    }

    private void consumeGauge(String name, Map<String, String> labels, long tsMillis, double value) {
        consumer.onMetricBegin(ru.yandex.monlib.metrics.MetricType.DGAUGE);
        consumeLabels(name, labels);
        consumer.onDouble(tsMillis, value);
        consumer.onMetricEnd();
    }

    private void consumeHistogram() {
        consumer.onMetricBegin(ru.yandex.monlib.metrics.MetricType.HIST_RATE);
        consumeLabels(histogramBuilder.getName(), histogramBuilder.getLabels());
        consumer.onHistogram(histogramBuilder.getTsMillis(), histogramBuilder.toSnapshot());
        consumer.onMetricEnd();

        histogramBuilder.clear();
    }
}
