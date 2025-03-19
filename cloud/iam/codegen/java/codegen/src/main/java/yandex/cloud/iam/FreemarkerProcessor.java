package yandex.cloud.iam;

import java.io.StringWriter;

import freemarker.template.Configuration;
import freemarker.template.Template;

public class FreemarkerProcessor {
    private static Configuration cfg = init();

    static Configuration init() {
        Configuration cfg = new Configuration(Configuration.VERSION_2_3_23);
        cfg.setDefaultEncoding("UTF-8");
        cfg.setOutputEncoding("UTF-8");
        cfg.setNumberFormat("computer");
        cfg.setClassForTemplateLoading(FreemarkerProcessor.class, "/");
        return cfg;
    }

    public static String processFreeMarkerTemplate(String templatePath, Object model) {
        try (StringWriter sw = new StringWriter()) {
            Template template = cfg.getTemplate(templatePath);
            template.process(model, sw);
            return sw.toString();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
