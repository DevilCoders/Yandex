import logging

import yaqutils.misc_helpers as umisc


# This class implements configuration of 'requirements' (attributes of SERP or serp component)
# https://wiki.yandex-team.ru/JandeksPoisk/Metrics/API/serp/#requirement
class SerpAttrs(object):
    def __init__(self, serp_reqs=None, component_reqs=None, sitelink_reqs=None, judgements=None):
        self.serp_reqs = serp_reqs or []
        self.component_reqs = component_reqs or []
        self.sitelink_reqs = sitelink_reqs or []
        self.judgements = judgements or []

        # if not self.judgements:
        #     logging.info("Setting default judgement list")
        #     self.judgements = [
        #         "RELEVANCE"
        #     ]
        logging.info("Required SERP attributes:")
        logging.info("--> serp-level: %s", self.serp_reqs)
        logging.info("--> component-level: %s", self.component_reqs)
        logging.info("--> sitelink-level: %s", self.sitelink_reqs)
        logging.info("--> judgements: %s", self.judgements)

        self.requirement_set = self.build_requirement_set()

    def get_requirements(self):
        return self.requirement_set

    def get_full_serp_req_names(self):
        serp_fields = ["{}".format(attr) for attr in self.serp_reqs]
        return set(serp_fields)

    def get_full_component_req_names(self):
        comp_fields = ["{}".format(attr) for attr in self.component_reqs]
        judgement_fields = ["judgements.{}".format(attr) for attr in self.judgements]
        sitelink_fields = ["site-links"] if self.sitelink_reqs else []

        return set(comp_fields + judgement_fields + sitelink_fields)

    @staticmethod
    def from_cli_args(cli_args):
        serp_reqs = umisc.parse_comma_separated_values(cli_args.serp_attrs)
        component_reqs = umisc.parse_comma_separated_values(cli_args.component_attrs)
        sitelink_reqs = umisc.parse_comma_separated_values(cli_args.sitelink_attrs)
        judgements = umisc.parse_comma_separated_values(cli_args.judgements)

        return SerpAttrs(serp_reqs=serp_reqs,
                         component_reqs=component_reqs,
                         sitelink_reqs=sitelink_reqs,
                         judgements=judgements)

    def build_requirement_set(self):
        mc_serp_reqs = ["SERP.{}".format(name) for name in self.serp_reqs]
        mc_comp_reqs = ["COMPONENT.{}".format(name) for name in self.component_reqs]
        mc_sitelink_reqs = ["SITELINK.{}".format(name) for name in self.sitelink_reqs]
        mc_judgement_reqs = ["COMPONENT.judgements.{}".format(name) for name in self.judgements]
        return set(mc_serp_reqs + mc_comp_reqs + mc_sitelink_reqs + mc_judgement_reqs)
