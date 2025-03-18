import datetime
import logging

import adminka.ab_cache
import adminka.activity
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
from adminka import date_validation as adm_dv
from experiment_pool import Pool


class PoolFilter(object):
    @staticmethod
    def from_cli_args(args, adminka_session=None):
        if adminka_session is None:
            adminka_session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(args.ab_token_file))

        return PoolFilter(
            aspect=args.aspect,
            queue_id=args.queue_id,
            dim_id=args.dim_id,
            service=args.service,
            platform=args.platform,
            regions=args.regions,
            min_duration=args.min_duration,
            max_duration=args.max_duration,
            remove_filtered=args.remove_filtered,
            remove_endless=args.remove_endless,
            remove_salt_changes=args.remove_salt_changes,
            only_regular_salt_changes=args.only_regular_salt_changes,
            only_main_observations=args.only_main_observations,
            tag_blacklist=args.remove_tags,
            tag_whitelist=args.tags,
            obs_blacklist=args.remove_observation_ids,
            obs_whitelist=args.observation_ids,
            adminka_session=adminka_session,
        )

    @staticmethod
    def add_cli_args(parser):
        parser.add_argument(
            "--aspect",
            help="quality aspect to filter observations on",
        )
        parser.add_argument(
            "--queue-id",
            type=int,
            help="queue ID to filter observations on",
        )
        parser.add_argument(
            "--dim-id",
            type=int,
            help="dimension ID to filter observations on",
        )
        parser.add_argument(
            "--remove-filtered",
            action="store_true",
            help="remove filtered observations (keep only integral ones)",
        )

        parser.add_argument(
            "--min-duration",
            type=int,
            help="minimum duration (in days) of observations to keep",
        )
        parser.add_argument(
            "--max-duration",
            type=int,
            help="maximum duration (in days) of observations to keep",
        )

        parser.add_argument(
            "--service",
            help="service to include",
        )
        parser.add_argument(
            "--platform",
            help="platform to include",
        )
        parser.add_argument(
            "--regions",
            type=int,
            nargs="+",
            help="region IDs to include",
        )
        parser.add_argument(
            "--remove-endless",
            action="store_true",
            help="remove observations with end date == null",
        )

        parser.add_argument(
            "--tags",
            nargs="+",
            help="only accept observations with ALL of these tags",
            default=[],
        )
        parser.add_argument(
            "--remove-tags",
            nargs="+",
            help="remove observations with ANY of these tags",
            default=[],
        )

        parser.add_argument(
            "--observation-ids",
            nargs="+",
            help="only accept observations with these ids",
            default=[],
        )
        parser.add_argument(
            "--remove-observation-ids",
            nargs="+",
            help="remove observations with these ids",
            default=[],
        )

        uargs.add_boolean_argument(
            parser,
            "--remove-salt-changes",
            help_message="remove observations that had salt changes",
        )
        uargs.add_boolean_argument(
            parser,
            "--only-regular-salt-changes",
            help_message="only keep observations that had regular salt changes",
        )
        uargs.add_boolean_argument(
            parser,
            "--only-main-observations",
            help_message="keep observation only if observation.task.observation_main == observation.id",
        )
        mstand_uargs.add_ab_token_file(parser)

    def __init__(self,
                 aspect=None,
                 queue_id=None,
                 dim_id=None,
                 service=None,
                 platform=None,
                 regions=None,
                 min_duration=None,
                 max_duration=None,
                 remove_filtered=False,
                 remove_endless=False,
                 remove_salt_changes=False,
                 only_regular_salt_changes=False,
                 only_main_observations=False,
                 tag_blacklist=None,
                 tag_whitelist=None,
                 obs_blacklist=None,
                 obs_whitelist=None,
                 adminka_session=None,
                 ):
        self.aspect = aspect
        self.queue_id = queue_id
        self.dim_id = dim_id

        self.service = service
        self.platform = platform
        self.regions = set(regions or [])
        self.min_duration = min_duration
        self.max_duration = max_duration
        self.remove_filtered = remove_filtered
        self.remove_endless = remove_endless

        self.remove_salt_changes = remove_salt_changes
        self.only_regular_salt_changes = only_regular_salt_changes
        self.only_main_observations = only_main_observations

        if remove_salt_changes and only_regular_salt_changes:
            raise Exception("--remove-salt-changes and --only-regular-salt-changes can't be used at the same time")

        self.session = adminka_session or adminka.ab_cache.AdminkaCachedApi()

        self.all_platforms = set()
        self.all_services = set()

        self.tag_blacklist = umisc.stripped_set(tag_blacklist)
        self.tag_whitelist = umisc.stripped_set(tag_whitelist)

        self.obs_blacklist = umisc.stripped_set(obs_blacklist)
        self.obs_whitelist = umisc.stripped_set(obs_whitelist)

        self.check_filter_fields()

    def check_filter_fields(self):
        known_services = {"appsearch", "avia", "cross_service", "images",
                          "mail", "maps", "market", "morda",
                          "pogoda", "touchsearch", "video", "web"}
        if self.service and self.service not in known_services:
            logging.warning("Unknown service in filter request: '%s'", self.service)

        known_platforms = {"desktop", "smart", "tablet", "touch"}
        if self.platform and self.platform not in known_platforms:
            logging.warning("Unknown platform in filter request: '%s'", self.platform)

    def log_enum_field_stats(self):
        if self.all_platforms:
            logging.info("All platforms in source pool: %s", sorted(map(str, self.all_platforms)))
        if self.all_services:
            logging.info("All services in source pool: %s", sorted(map(str, self.all_services)))

    def need_preload_testids(self):
        return self.queue_id is not None

    def check_aspect(self, observation):
        if not observation.id:
            return True
        task = self.session.get_task_info_for_observation(observation.id)
        if task:
            aspect = task.get("aspect")
            if aspect == self.aspect:
                return True
            else:
                logging.info(
                    "Removed observation %s: requested aspect %s, got %s",
                    observation.id,
                    self.aspect,
                    aspect
                )
                return False
        else:
            logging.info("Removed observation %s: no task associated with observation (aspect check)")
            return False

    def check_main_observation(self, observation):
        if not observation.id:
            logging.info("Removed observation without id: %s", observation)
            return False
        task = self.session.get_task_info_for_observation(observation.id)
        if not task:
            logging.info("Removed observation %s without task", observation.id)
            return False
        obs_main = task.get("observation_main")
        if not obs_main:
            logging.info("Removed observation %s without task.observation_main", observation.id)
            return False
        obs_id = str(observation.id)
        if obs_main != ("abt" + obs_id):
            logging.info("Removed observation %s: task.observation_main = %s", observation.id, obs_main)
            return False
        return True

    def check_queue_id(self, observation):
        for testid in observation.all_testids():
            ti = self.session.get_testid_info(testid)
            queue_id = ti.get("queue_id")
            if queue_id != self.queue_id:
                logging.info(
                    "Removed observation %s: requested queue_id %s, testid %s has %s",
                    observation.id,
                    self.queue_id,
                    testid,
                    queue_id
                )
                return False
        return True

    def _get_footprints(self, observation):
        for testid in observation.all_testids():
            activity = self.session.get_testid_activity(testid)
            pair = adminka.activity.closest_enabled_event_pair(activity, observation.dates)
            if pair is not None:
                for footprint in pair.on.get("footprints", []):
                    yield footprint

    # restrictions: {
    #     'percent': 5.0,
    #     'devices': '',
    #     'regions': '225,187,149,159,983',
    #     'platforms': 'smart,tablet,touch',
    #     'browsers': '',
    #     'services': 'gorsel,images,msearch,padsearch,touch,video,web',
    #     'networks': ''
    # }

    def _get_restrictions(self, observation):
        for footprint in self._get_footprints(observation):
            if "restrictions" in footprint:
                yield footprint["restrictions"]

    def check_dim_id(self, observation):
        all_dim_ids = set()
        for footprint in self._get_footprints(observation):
            dim_id = footprint.get("dim_id")
            if dim_id == self.dim_id:
                return True
            elif dim_id:
                all_dim_ids.add(dim_id)

        logging.info(
            "Removed observation %s: requested dim_id %s, got %s",
            observation.id,
            self.dim_id,
            ", ".join(map(str, sorted(all_dim_ids)))
        )

        return False

    def check_service(self, observation):
        if not observation.id:
            return True
        task = self.session.get_task_info_for_observation(observation.id)
        if task:
            obs_service = task.get("aspect_group")

            if obs_service:
                self.all_services.add(obs_service)

            if self.service == obs_service:
                return True
            else:
                logging.info(
                    "Removed observation %s: requested service %s, got %s",
                    observation.id,
                    self.service,
                    obs_service
                )
                return False
        else:
            logging.info("Removed observation %s: no task associated with observation (service check)", observation.id)
        return False

    def get_all_platforms(self, observation):
        restrictions = list(self._get_restrictions(observation))
        all_platforms = set()

        for restriction in restrictions:
            obs_platforms = restriction.get("platforms", "").split(",")
            all_platforms.update(obs_platforms)

        if not all_platforms or "" in all_platforms:
            return set()
        return all_platforms

    def get_uitype_filters(self, observation):
        obs_info = self.session.get_observation_info(observation.id)
        uitype_filters = set()

        for filter_node in obs_info["filters"]:
            if filter_node["name"] == "uitype_filter":
                uitype_filters.update(filter_node["value"].split(" "))
        return uitype_filters

    def check_platform(self, observation):
        all_platforms = self.get_all_platforms(observation)
        uitype_filters = self.get_uitype_filters(observation)

        if not all_platforms and not uitype_filters:
            return True

        if all_platforms and uitype_filters:
            intersection = all_platforms & uitype_filters
        else:
            intersection = all_platforms | uitype_filters

        self.all_platforms.update(intersection)

        if self.platform in intersection:
            return True

        logging.info(
            "Removed observation %s: requested platform %s, got %s",
            observation.id,
            self.platform,
            sorted(all_platforms),
        )
        return False

    def check_regions(self, observation):
        restrictions = list(self._get_restrictions(observation))

        if not restrictions:
            return True

        all_regions = set()
        for restriction in restrictions:
            raw_regions = restriction.get("regions", "").split(",")
            obs_regions = set([int(region) for region in raw_regions if region])
            all_regions.update(obs_regions)
            if obs_regions & self.regions:
                return True

        logging.info(
            "Removed observation %s: requested regions %s, got %s",
            observation.id,
            ", ".join(map(str, self.regions)),
            ", ".join(map(str, all_regions))
        )

        return False

    def check_tags(self, observation):
        if observation.tags:
            obs_tags = set(observation.tags)
        else:
            obs_tags = set()
        bad_tags = obs_tags.intersection(self.tag_blacklist)
        if bad_tags:
            logging.info("Removed observation %s: has blacklisted tags: %s", observation.id, ", ".join(bad_tags))
            return False
        missing_tags = self.tag_whitelist.difference(obs_tags.intersection(self.tag_whitelist))
        if missing_tags:
            logging.info("Removed observation %s: missing requested tags: %s", observation.id, ", ".join(missing_tags))
            return False
        return True

    def check_id(self, observation):
        if not observation.id:
            if self.obs_whitelist:
                logging.info("Removed observation %s: no id", observation)
                return False
            return True
        if observation.id in self.obs_blacklist:
            logging.info("Removed observation %s: ids blacklist", observation)
            return False
        if self.obs_whitelist and observation.id not in self.obs_whitelist:
            logging.info("Removed observation %s: ids whitelist", observation)
            return False
        return True

    def check_salt_changes(self, observation):
        split_changes = self.session.get_split_change_info(observation.all_testids(), observation.dates)

        if split_changes and self.remove_salt_changes:
            logging.info(
                "Removed observation %s: split changes at %s",
                observation, ", ".join(sorted(split_changes.keys()))
            )
            return False

        if self.only_regular_salt_changes:
            if not split_changes:
                logging.info("Removed observation %s: no split changes", observation)
                return False

            for date, change in split_changes.items():
                for testid, change_types in change.items():
                    if not change_types["regular"]:
                        logging.info(
                            "Removed observation %s: not regular split change on %s for testid %s",
                            observation, date, testid
                        )
                        return False

        return True

    def is_accepted(self, observation):
        if (self.obs_whitelist or self.obs_blacklist) and not self.check_id(observation):
            return False

        if self.remove_endless and observation.dates.end is None:
            logging.info("Removed observation %s: is endless", observation)
            return False

        obs_duration = observation.dates.number_of_days()
        if self.min_duration is not None and obs_duration < self.min_duration:
            logging.info(
                "Removed observation %s: requested min duration %s, observation duration: %s",
                observation.id,
                self.min_duration,
                obs_duration
            )
            return False

        if self.max_duration is not None and obs_duration > self.max_duration:
            logging.info(
                "Removed observation %s: requested max duration %s, observation duration: %s",
                observation.id,
                self.max_duration,
                obs_duration
            )
            return False

        if self.aspect is not None and not self.check_aspect(observation):
            return False

        if self.queue_id is not None and self.queue_id > 0 and not self.check_queue_id(observation):
            return False

        if self.dim_id is not None and not self.check_dim_id(observation):
            return False

        if self.service and not self.check_service(observation):
            return False

        if self.platform and not self.check_platform(observation):
            return False

        if self.regions and not self.check_regions(observation):
            return False

        if self.remove_filtered and observation.id and self.session.get_observation_info(observation.id).get("filters"):
            logging.info("Removed observation %s: is filtered", observation)
            return False

        if (self.remove_salt_changes or self.only_regular_salt_changes) and not self.check_salt_changes(observation):
            return False

        if (self.tag_whitelist or self.tag_blacklist) and not self.check_tags(observation):
            return False

        if self.only_main_observations and not self.check_main_observation(observation):
            return False

        logging.debug("Kept observation %s", observation)
        return True

    def filter(self, pool):
        """
        :type pool: Pool
        :return: Pool
        """
        logging.info("Filtering: preloading testids")
        if self.need_preload_testids():
            logging.info("Queue ID filtering requested, preloading testID info...")
            self.session.preload_testids(pool.all_testids())
            logging.debug("Preloaded all testIDs")
        logging.info("Filtering: performing filtration itself")
        return Pool(list(filter(self.is_accepted, pool.observations)))


def min_date(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return min(a, b)


def trim_days_single(observation, day_from=None, day_to=None):
    # TODO: handle negative days
    if day_to and day_to > 0:
        observation.dates.end = min_date(observation.dates.end,
                                         observation.dates.start + datetime.timedelta(days=day_to - 1))
    if day_from and day_from > 0:
        observation.dates.start = min_date(observation.dates.end,
                                           observation.dates.start + datetime.timedelta(days=day_from - 1))
    return observation


def trim_days(observations, trim_from, trim_to, keep_truncated):
    """
    WARNING: observations argument is mutable

    :type observations: list[Observation]
    :type trim_from: int
    :type trim_to: int
    :type keep_truncated: bool
    """
    if trim_to and not keep_truncated:
        min_duration = trim_to
    elif trim_from:
        min_duration = trim_from
    else:
        min_duration = 1

    observations[:] = [trim_days_single(o, day_from=trim_from, day_to=trim_to)
                       for o in observations
                       if o.dates.number_of_days() >= min_duration]
    return observations


def split_overlap(observations, length, step):
    """
    WARNING: observations argument is mutable

    :type observations: list[Observation]
    :type length: int
    :type step: int
    """
    assert length > 0
    assert step > 0
    new_observations = []
    for obs in observations:
        if obs.dates.is_finite():
            date_from = obs.dates.start
            date_to = date_from + datetime.timedelta(days=length - 1)
            while date_to <= obs.dates.end:
                new_obs = obs.clone()
                new_obs.dates = utime.DateRange(date_from, date_to)
                new_observations.append(new_obs)
                date_from += datetime.timedelta(days=step)
                date_to += datetime.timedelta(days=step)
    observations[:] = new_observations
    return observations


def ensure_full_days(observation, session):
    logging.info("-> Checking observation %s for full days", observation)

    enabled_ranges = adm_dv.get_all_enabled_ranges(
        session=session,
        testids=observation.all_testids()
    )

    old_dates = utime.DateRange(observation.dates.start, observation.dates.end)

    for en_range in enabled_ranges:
        logging.debug("  -> checking enabled range %s", en_range)
        if en_range.intersect(observation.dates) == observation.dates:
            logging.debug("    -> range matches observation dates")

            if en_range.start == observation.dates.start:
                logging.info("    -> Start day may be not full, moving")
                observation.dates.start += datetime.timedelta(days=1)

            if en_range.end == observation.dates.end:
                logging.info("    -> End day may be not full, moving")
                observation.dates.end -= datetime.timedelta(days=1)

            if old_dates == observation.dates:
                logging.info(" -> Check passed, dates not changed")
            else:
                logging.info(" -> Saving new dates: %s", observation.dates)

            return observation
    else:
        logging.warning(" -> No valid date ranges found, removed!")


def ensure_full_days_all(observations, session):
    new_observations = []
    for observation in observations:
        fixed = ensure_full_days(observation, session)
        if fixed:
            new_observations.append(fixed)
    return new_observations


def generate_new_date_ranges(dates, change_dates):
    sorted_dates = sorted(change_dates)

    last_day_before_first_change = sorted_dates[0] - datetime.timedelta(days=1)

    if last_day_before_first_change >= dates.start:
        dr = utime.DateRange(dates.start, last_day_before_first_change)
        logging.info(" -> Date range from start to first split: %s", dr)
        yield dr

    for start, end in zip(sorted_dates, sorted_dates[1:]):
        start += datetime.timedelta(days=1)
        end -= datetime.timedelta(days=1)
        if start <= end:
            dr = utime.DateRange(start, end)
            logging.info(" -> Date range between splits: %s", dr)
            yield dr
        else:
            logging.info(" -> Date range between splits too short: %s -%s", start, end)

    first_day_after_last_change = sorted_dates[-1] + datetime.timedelta(days=1)
    if first_day_after_last_change <= dates.end:
        dr = utime.DateRange(first_day_after_last_change, dates.end)
        logging.info(" -> Date range from last split to end: %s", dr)
        yield dr


def split_at_salt_changes(observation, session):
    salt_changes = session.get_split_change_info(observation.all_testids(), observation.dates)

    logging.info("-> Splitting observation %s at salt change dates", observation)

    if not salt_changes:
        logging.info(" -> No salt changes, will not split")
        yield observation
        return

    salt_change_dates = sorted(set(utime.parse_date_msk(date) for date in salt_changes))
    logging.info(" -> Salt change dates: %s", ", ".join(map(str, salt_change_dates)))

    for dr in generate_new_date_ranges(observation.dates, salt_change_dates):
        new_obs = observation.clone(clone_experiments=True)
        new_obs.dates = dr
        yield new_obs


def split_at_salt_changes_all(observations, session):
    new_observations = []
    for observation in observations:
        new_observations.extend(split_at_salt_changes(observation, session))
    return new_observations
