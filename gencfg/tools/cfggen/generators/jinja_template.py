from collections import defaultdict
import jinja2
import os

import gaux.aux_hbf
import tools.cfggen.generators
import tools.cfggen.generators.common as common


class JinjaTemplateGenerator(tools.cfggen.generators.IGenerator):
    def __init__(self):
        super(JinjaTemplateGenerator, self).__init__()
        self.jinja_env = None
        self.template_data = None

    def build(self, group):
        result = tools.cfggen.generators.TBuildReport(group)

        self.jinja_env = self._create_jinja_env(group.parent.db.get_path())
        self._prepare_template_values(group)

        rendered_configs = defaultdict(list)

        for config_type in group.card.configs.jinja_template:
            for config, filenames in self._render_configs(config_type, group).iteritems():
                rendered_configs[config].extend(filenames)

        if 'workloads' in group.card:
            # Generate configs for each workload instance
            for workload in group.card.workloads:
                for config_type in workload.configs:
                    if 'template' not in config_type:
                        continue

                    for config, filenames in self._render_configs(config_type, group, workload_name=workload.name).iteritems():
                        rendered_configs[config].extend(filenames)

        for config_text, config_files in rendered_configs.iteritems():
            result.report_entries.append(tools.cfggen.generators.TBuildReportEntry(config_text, config_files))

        return result

    def _prepare_template_values(self, group):
        self.template_data = common.get_template_data(group)
        self.template_data.setdefault('group', group)

        if 'jinja_params' in group.card.configs:
            self.template_data.update({
                jinja_param.key: jinja_param.value
                for jinja_param in group.card.configs.jinja_params
            })

    def _create_jinja_env(self, db_path):
        return jinja2.Environment(
            loader=jinja2.FileSystemLoader(
                os.path.join(db_path, "configs", "jinja_template")
            )
        )

    def _render_configs(self, config_type, group, workload_name=None):
        jinja_template = self.jinja_env.get_template(config_type.template)

        configs = defaultdict(set)
        shard_numbers = common.get_shard_numbers(group)

        if 'config_name' in config_type:
            config_text = jinja_template.render(
                instance=None,
                workload_name=workload_name,
                **self.template_data
            ) + "\n"

            configs[config_text].update([config_type.config_name])

        if 'file_name_template' in config_type:
            for instance in group.get_kinda_busy_instances():
                filename_data = {}
                if workload_name:
                    filename_data['WORKLOAD'] = workload_name

                filename_data['shard_number'] = shard_numbers.get(instance)

                file_names = [
                    config_type.file_name_template.format(INSTANCE=instance_name, **filename_data)
                    for instance_name in self._get_instance_names(group, instance)
                ]

                config_text = jinja_template.render(
                    instance=instance,
                    workload_name=workload_name,
                    shard_number=shard_numbers.get(instance),
                    **self.template_data
                ) + "\n"

                configs[config_text].update(file_names)

        return configs

    def _get_instance_names(self, group, instance):
        names = [instance.short_name()]

        if group.card.properties.mtn.use_mtn_in_config:
           host = gaux.aux_hbf.generate_mtn_hostname(instance, group, '')
           names.append('{}:{}'.format(host, instance.port))

        return names

    def gennames(self, group, add_custom_name=True):
        instances = group.get_kinda_busy_instances()
        config_names = map(lambda x: "%s.cfg" % x.short_name(), instances)
        if add_custom_name and group.card.configs.basesearch.custom_name:
            config_names.append(group.card.configs.basesearch.custom_name)

        return config_names
