package yandex.cloud.dashboard.model.result.dashboard;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * @author karelinoleg
 */
@AllArgsConstructor
@Data
public class Options {
    List<Link> dataLinks;
}
