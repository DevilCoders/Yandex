import os

import custom_generators.intmetasearchv2.config_template
import node


class TFileNode(node.INode):
    INTERNAL_KEYS = []

    def __init__(self, fname, kvstore):
        super(TFileNode, self).__init__([], fname, None, kvstore, dict())

    def render_to_files(self, thread_id, total_threads, queue, options):
        """
        Parallel rendering. Generate configs for every <thread_id>'th config template.

        :type queue: multiprocessing.Queue or None
        :param thread_id: current 'thread id' (int in range [0, <total_threads>))
        :param total_threads: total processes count
        :param queue: communication queue with master process
        :param options: command-line options as result of core.arparse.parser.ArgumentParserExt
        """
        result = node.TRenderConfigsResult(thread_id)

        try:
            self.children = self._replace_config_templates(thread_id, total_threads, config_names=options.config_names)

            for config_node in self._get_config_nodes(options):
                rendered = config_node.render()
                for filename in config_node.filenames:
                    with open(os.path.join(options.output_dir, filename), 'w') as f:
                        f.write("# %s\n%s" % (filename, rendered))

                    if options.extra_output_dir and any(anchor in config_node.get_anchor_name() for anchor in options.anchors):
                        with open(os.path.join(options.extra_output_dir, filename), 'w') as f:
                            f.write("# %s\n%s" % (filename, rendered))

                    result.generated_configs.append(os.path.join(options.output_dir, filename))
                result.config_nodes_count += 1
        finally:
            if queue is not None:
                queue.put(result)
                queue.close()

        return result

    def render(self, strict=True):
        return "File %s\n%s" % (self.name, self.render_childs(strict=strict))

    def _replace_config_templates(self, thread_id, total_threads, config_names):
        """
            We have list of different things in children: TConfigTemplateNodes among them.
            Here we replace every instance of TConfigTemplateNode by bunch of TConfigNodes

            :param thread_id: current 'thread id' (int in range [0, <total_threads>))
            :param total_threads: total processes count
            :param config_names: comma-separated list of configs to generate
        """

        new_children = []
        for child_id, child in enumerate(self.children):
            if isinstance(child, custom_generators.intmetasearchv2.config_template.TConfigTemplateNode):
                if child_id % total_threads == thread_id:
                    if config_names is None or (child.filename is not None and child.filename in config_names):
                        new_children.extend(child.generate_config_nodes())
            else:
                new_children.append(child)

        return new_children

    def _get_config_nodes(self, options):
        for child in self.children:
            if not isinstance(child, custom_generators.intmetasearchv2.config_template.TConfigNode):
                continue

            if options.config_names:
                if child.filenames is not None and (set(child.filenames) & set(options.config_names)):
                    yield child
            else:
                yield child
