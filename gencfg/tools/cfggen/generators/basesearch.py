from collections import defaultdict
import jinja2
import os

import gaux.aux_hbf
import tools.cfggen.generators as generators
import tools.cfggen.generators.common as common


class TBasesearchGenerator(generators.IGenerator):
    def __init__(self):
        super(TBasesearchGenerator, self).__init__()
        self.template_data = {}

    def build(self, group):
        result = generators.TBuildReport(group)

        self._prepare_template_values(group)

        if 'template' in group.card.configs.basesearch:
            for rendered_config, config_files in self._render_xml_template(group).iteritems():
                result.report_entries.append(generators.TBuildReportEntry(rendered_config, config_files))

        if 'protoconfig' in group.card.configs.basesearch:
            for rendered_config, config_files in self._render_protoconfigs(group).iteritems():
                result.report_entries.append(generators.TBuildReportEntry(rendered_config, config_files))

        return result

    def _prepare_template_values(self, group):
        self.template_data = common.get_template_data(group)

        self.template_data.update({
            jinja_param.key: jinja_param.value
            for jinja_param in group.card.configs.basesearch.jinja_params
        })

    def _render_xml_template(self, group):
        return self._render_text_with_jinja(group, group.card.configs.basesearch.template, "{}.cfg")

    def _render_protoconfigs(self, group):
        return self._render_text_with_jinja(group, group.card.configs.basesearch.protoconfig, "{}.cfg.proto")

    def _render_text_with_jinja(self, group, config_template_name, config_name_format):
        template_dir = os.path.join(group.parent.db.get_path(), "configs", "basesearch", "templates")
        env = jinja2.Environment(loader=jinja2.FileSystemLoader(template_dir))

        jinja_template = env.get_template(config_template_name)

        config_bodies = defaultdict(list)

        # Single config for the group.
        if group.card.configs.basesearch.config_name and not config_name_format.endswith('proto'):
            config_body = jinja_template.render(instance=None, group=group, **self.template_data) + "\n"
            config_bodies[config_body].append(group.card.configs.basesearch.config_name)
            # Generate sample config for backward compatibility.
            if group.card.configs.basesearch.sample_name and not config_name_format.endswith('proto'):
                config_bodies[config_body].append(group.card.configs.basesearch.sample_name)
        else:
            # Instancewise configs.
            for instance in group.get_kinda_busy_instances():
                config_body = jinja_template.render(instance=instance, group=group, **self.template_data) + "\n"

                config_names = [config_name_format.format(instance.short_name())]
                if group.card.properties.mtn.use_mtn_in_config:
                    host = gaux.aux_hbf.generate_mtn_hostname(instance, group, '')
                    mtn_instance = '{}:{}'.format(host, instance.port)
                    config_names.append(config_name_format.format(mtn_instance))

                config_bodies[config_body].extend(config_names)

            if group.card.configs.basesearch.custom_name:
                if len(config_bodies) <= 1:
                    for config_body, config_names in config_bodies.iteritems():
                        config_names.append(group.card.configs.basesearch.custom_name)
                else:
                    raise RuntimeError(
                        'Group %s has %s different configs, could not add %s config',
                        group.card.name,
                        len(config_bodies),
                        group.card.configs.basesearch.custom_name
                    )

            # Take one instance config as a sample.
            if group.card.configs.basesearch.sample_name and not config_name_format.endswith('proto'):
                if config_bodies:
                    config_bodies.itervalues().next().append(group.card.configs.basesearch.sample_name)

        return config_bodies

    def gennames(self, group, add_custom_name=True):
        instances = group.get_kinda_busy_instances()
        config_names = map(lambda x: "%s.cfg" % x.short_name(), instances)
        if add_custom_name and group.card.configs.basesearch.custom_name:
            config_names.append(group.card.configs.basesearch.custom_name)

        return config_names
