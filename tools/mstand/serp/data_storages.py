import os
import logging
import shutil

import yaqutils.file_helpers as ufile
import yaqutils.six_helpers as usix

pjoin = os.path.join

CACHE_SUBDIR = "work_dir"


class DataStorage(object):
    def __init__(self, root_dir=".", cache_subdir=CACHE_SUBDIR, use_cache=True):
        """
        :type root_dir: str
        :type cache_subdir: str
        :type use_cache: bool
        """
        # root directory for cache - expected to exist
        if not root_dir and not cache_subdir:
            raise Exception("Params 'root_dir' and 'cache_subdir' in DataStorage could not be empty. ")

        self.root_dir = root_dir
        self.cache_subdir = cache_subdir
        self.work_dir = pjoin(root_dir, cache_subdir)
        self.use_cache = use_cache

        # main directory where offline metric temporary files are stored
        ufile.make_dirs(self.work_dir)

    def unpack_from_tar(self, tar_file_name):
        ufile.untar_directory(tar_file_name, dest_dir=self.root_dir, verbose=False)

    def pack_to_tar(self, tar_file_name):
        ufile.tar_directory(path_to_pack=self.base_dir(), tar_name=tar_file_name, dir_content_only=False, verbose=False)

    def _expand(self, rel_path):
        return pjoin(self.work_dir, rel_path)

    def base_dir(self):
        return self.work_dir

    def pool_path(self):
        return self._expand(DataStorage.pool_filename())

    @staticmethod
    def pool_filename():
        return "pool.json"


class RawSerpDataStorage(DataStorage):
    def __init__(self, root_dir=".", cache_subdir=CACHE_SUBDIR, use_cache=True, raw_serpset_dir_name="raw_serpsets"):
        """
        :type root_dir: str
        :type cache_subdir: str
        :type use_cache: bool
        """
        super(RawSerpDataStorage, self).__init__(root_dir, cache_subdir, use_cache)
        self.raw_serpset_dir_name = raw_serpset_dir_name
        ufile.make_dirs(self.raw_serpset_dir())

    def display_contents(self, serpset_ids):
        """
        :type serpset_ids: set[str]
        :rtype:
        """
        for serpset_id in serpset_ids:
            jsonlines_serpset_file = self.jsonlines_serpset_by_id(serpset_id)
            if os.path.exists(jsonlines_serpset_file):
                logging.info("Raw serpset in cache: %s", jsonlines_serpset_file)

    def cleanup(self):
        raw_serp_dir = self.raw_serpset_dir()
        if os.path.exists(raw_serp_dir):
            logging.info("Removing raw serpsets dir: %s", raw_serp_dir)
            shutil.rmtree(raw_serp_dir)

    def should_convert_serpset(self, serpset_id):
        if not self.use_cache:
            return True

        jsonlines_serpset_file_name = self.jsonlines_serpset_by_id(serpset_id)
        if ufile.is_file_or_link(jsonlines_serpset_file_name):
            logging.info("Converted .jsonl serpset %s found in cache, skipping conversion.", serpset_id)
            return False

        logging.info("No cached .jsonl serpset %s, converting it.", serpset_id)
        return True

    def should_download_serpset(self, serpset_id: str):
        if not self.use_cache:
            return True

        jsonlines_serpset_file_name = self.jsonlines_serpset_by_id(serpset_id)
        if ufile.is_not_empty_file(jsonlines_serpset_file_name):
            logging.info("jsonlines serpset %s found in cache, not downloading.", serpset_id)
            return False

        raw_serpset_file_name = self.raw_gz_serpset_by_id(serpset_id)
        if ufile.is_not_empty_file(raw_serpset_file_name):
            logging.info("Raw serpset %s is in cache, not downloading.", serpset_id)
            return False

        logging.info("No cached raw/jsonlines serpset %s, downloading it.", serpset_id)
        return True

    def raw_serpset_dir(self):
        return self._expand(self.raw_serpset_dir_name)

    def _rel_raw_gz_serpset_by_id(self, serpset_id):
        return pjoin(self.raw_serpset_dir_name, "serpset_{}.json.gz".format(serpset_id))

    def _rel_serpset_meta_by_id(self, serpset_id):
        return pjoin(self.raw_serpset_dir_name, "meta_{}.json".format(serpset_id))

    def raw_gz_serpset_by_id(self, serpset_id):
        return self._expand(self._rel_raw_gz_serpset_by_id(serpset_id))

    def serpset_meta_by_id(self, serpset_id):
        return self._expand(self._rel_serpset_meta_by_id(serpset_id))

    def _rel_raw_json_serpset_by_id(self, serpset_id):
        return pjoin(self.raw_serpset_dir_name, "serpset_{}.json".format(serpset_id))

    def raw_json_serpset_by_id(self, serpset_id):
        return self._expand(self._rel_raw_json_serpset_by_id(serpset_id))

    def _rel_jsonlines_serpset_by_id(self, serpset_id):
        return pjoin(self.raw_serpset_dir_name, "serpset_{}.jsonl".format(serpset_id))

    def jsonlines_serpset_by_id(self, serpset_id):
        return self._expand(self._rel_jsonlines_serpset_by_id(serpset_id))

    @staticmethod
    def from_cli_args(cli_args):
        return RawSerpDataStorage(root_dir=cli_args.root_cache_dir,
                                  cache_subdir=cli_args.cache_subdir,
                                  use_cache=cli_args.use_cache)


class ParsedSerpDataStorage(DataStorage):
    def __init__(self, root_dir: str = ".", cache_subdir: str = CACHE_SUBDIR, use_cache: bool = True,
                 parsed_serpsets_dir_name="parsed_serpsets"):
        super(ParsedSerpDataStorage, self).__init__(root_dir, cache_subdir, use_cache)
        self.parsed_serpsets_dir_name = parsed_serpsets_dir_name
        ufile.make_dirs(self.parsed_serpset_dir())

    def is_already_parsed(self, serpset_id):
        queries_file = self.queries_by_serpset(serpset_id)
        markup_file = self.markup_by_serpset(serpset_id)

        return ufile.is_not_empty_file(queries_file) and ufile.is_not_empty_file(markup_file)

    def parsed_serpset_dir(self):
        return self._expand(self.parsed_serpsets_dir_name)

    def _rel_queries_by_serpset(self, serpset_id):
        return pjoin(self.parsed_serpsets_dir_name, "queries_{}.jsonl".format(serpset_id))

    def queries_by_serpset(self, serpset_id):
        return self._expand(self._rel_queries_by_serpset(serpset_id))

    def _rel_markup_by_serpset(self, serpset_id):
        return pjoin(self.parsed_serpsets_dir_name, "markup_{}.jsonl".format(serpset_id))

    def markup_by_serpset(self, serpset_id):
        return self._expand(self._rel_markup_by_serpset(serpset_id))

    def _rel_urls_by_serpset(self, serpset_id):
        return pjoin(self.parsed_serpsets_dir_name, "urls_{}.jsonl".format(serpset_id))

    def urls_by_serpset(self, serpset_id):
        return self._expand(self._rel_urls_by_serpset(serpset_id))

    @staticmethod
    def from_cli_args(cli_args):
        return ParsedSerpDataStorage(root_dir=cli_args.root_cache_dir,
                                     cache_subdir=cli_args.cache_subdir,
                                     use_cache=cli_args.use_cache)


class MetricDataStorage(DataStorage):
    def __init__(self, root_dir=".", cache_subdir=CACHE_SUBDIR, use_cache=True,
                 metric_dir_name="metric_results", metric_dir_name_external="metric_external"):
        """
        :type cache_subdir: str
        :type use_cache: bool
        """
        super(MetricDataStorage, self).__init__(root_dir, cache_subdir, use_cache)
        self.metric_dir_name = metric_dir_name
        self.metric_dir_name_external = metric_dir_name_external

        ufile.make_dirs(self.metric_dir())
        ufile.make_dirs(self.metric_dir_external())

    def is_serpset_computed(self, metric_keys, serpset_id, use_int_out, use_ext_out):
        """
        :type metric_keys: dict[int, PluginKey]
        :type serpset_id: str
        :type use_int_out: bool
        :type use_ext_out: bool
        :rtype: bool
        """
        if not self.use_cache:
            return False

        if use_ext_out:
            metric_result_file_external = self.metric_by_serpset_external(serpset_id)
            if not ufile.is_not_empty_file(metric_result_file_external):
                logging.info("No external metric result on serpset %s, calculating.", serpset_id)
                return False

        if not use_int_out:
            logging.info("No internal result requested, skipping internal files check")
        else:
            # check if metric results exist for metric instances
            for metric_id, metric_key in usix.iteritems(metric_keys):
                metric_result_file = self.metric_by_serpset(metric_id, serpset_id)

                if not ufile.is_not_empty_file(metric_result_file):
                    logging.info("No metric result for %s on serpset %s, calculating.", metric_key, serpset_id)
                    return False
        return True

    def rel_metric_by_serpset(self, key, serpset_id):
        metric_file_name = MetricDataStorage.metric_by_serpset_filename(key, serpset_id)
        return pjoin(self.metric_dir_name, metric_file_name)

    def rel_metric_by_serpset_external(self, serpset_id):
        metric_file_name = MetricDataStorage.metric_by_serpset_filename_external(serpset_id)
        return pjoin(self.metric_dir_name_external, metric_file_name)

    @staticmethod
    def metric_by_serpset_filename(key, serpset_id):
        return "metric_{}_serpset_{}.tsv".format(key, serpset_id)

    @staticmethod
    def metric_by_serpset_filename_external(serpset_id):
        # to avoid renaming for external Metrics output (METRICS-415, MSTAND-1143)
        return "{}.json".format(serpset_id)

    def metric_by_serpset(self, metric_id, serpset_id):
        """
        :type metric_id: int
        :param serpset_id: str
        :rtype: str
        """
        return self._expand(self.rel_metric_by_serpset(metric_id, serpset_id))

    def metric_by_serpset_external(self, serpset_id):
        return self._expand(self.rel_metric_by_serpset_external(serpset_id))

    def metric_dir(self):
        return self._expand(self.metric_dir_name)

    def metric_dir_external(self):
        return self._expand(self.metric_dir_name_external)

    @staticmethod
    def from_cli_args(cli_args):
        return MetricDataStorage(root_dir=cli_args.root_cache_dir,
                                 cache_subdir=cli_args.cache_subdir,
                                 use_cache=cli_args.use_cache)
