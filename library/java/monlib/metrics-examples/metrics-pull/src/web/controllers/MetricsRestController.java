package ru.yandex.monlib.metrics.example.pull.web.controllers;

import java.io.ByteArrayOutputStream;
import java.util.List;

import io.swagger.annotations.Api;
import io.swagger.annotations.ApiOperation;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.monlib.metrics.CompositeMetricSupplier;
import ru.yandex.monlib.metrics.MetricSupplier;
import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.encode.MetricFormat;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;

import static io.netty.handler.codec.http.HttpHeaderNames.CONTENT_TYPE;
import static ru.yandex.monlib.metrics.encode.MetricEncoderFactory.createEncoder;

/**
 * Controller provide pull metric endpoint
 *
 * @author Alexey Trushkin
 */
@Api(tags = {"metrics"})
@RestController
@RequestMapping(path = "/api/v1/metrics", produces = MediaType.APPLICATION_JSON_VALUE)
public class MetricsRestController {

    private List<MetricSupplier> suppliers;

    public MetricsRestController(List<MetricSupplier> suppliers) {
        this.suppliers = suppliers;
    }

    @ApiOperation(value = "Get metrics by pull", response = byte[].class)
    @GetMapping
    public ResponseEntity<byte[]> metrics(
            @RequestParam(value = "format", defaultValue = "TEXT") MetricFormat format,
            @RequestParam(value = "compression", defaultValue = "NONE") CompressionAlg compression) {
        return encode(new CompositeMetricSupplier(suppliers), format, compression);
    }

    public static ResponseEntity<byte[]> encode(MetricSupplier supplier, MetricFormat format, CompressionAlg compression) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(8 << 10); // 8 KiB
        try (MetricEncoder encoder = createEncoder(out, format, compression)) {
            supplier.supply(0, encoder);
        } catch (Exception e) {
            throw new IllegalStateException("cannot encode metrics", e);
        }
        return ResponseEntity.ok()
                .header(CONTENT_TYPE.toString(), format.contentType())
                .body(out.toByteArray());
    }

}
