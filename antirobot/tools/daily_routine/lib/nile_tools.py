# -*- coding: utf-8 -*-
import abc
import string
import random

from nile.api.v1 import clusters


class StreamConverter(object):
    """
    Nile wrapper. Класс, который по __call__ забирает несколько стримов и возвращает несколько стримов.
    """
    __metaclass__ = abc.ABCMeta

    def test(self, *inputs, **options):
        cluster = clusters.MockCluster()
        job = cluster.job()
        options["job"] = job
        options["cluster"] = cluster

        input_streams = [job.table("").debug_input(di) for di in inputs]
        output_streams = self(*input_streams, **options)
        debug_outputs = [list() for _ in range(len(output_streams))]

        for output_stream, debug_output in zip(output_streams, debug_outputs):
            output_stream.debug_output(debug_output).put("")
        job.debug_run()
        return debug_outputs

    def run(self, cluster, input_tables, output_tables, **options):
        tmp_root = "//tmp/ohmmeter/" + "".join(random.sample(string.letters, 8))
        cluster_upgraded = cluster.env(
            parallel_operations_limit=5,
            templates={"tmp_root": tmp_root},
        )
        job = cluster_upgraded.job()
        options["job"] = job
        options["cluster"] = cluster_upgraded

        input_streams = [job.table(table) for table in input_tables]
        output_streams = self(*input_streams, **options)
        for output_stream, table in zip(output_streams, output_tables):
            output_stream.put(table)

        with cluster.driver.transaction():
            job.run()

    @abc.abstractmethod
    def __call__(self, *args, **kwargs):
        pass
