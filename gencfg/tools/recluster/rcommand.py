"""
    Structures, describing single recluster command
"""

import jinja2
import shlex

import gaux.aux_utils
from config import MAIN_DIR
from gaux.aux_colortext import green_text, dblue_text


class TReclusterCommand(object):
    """
        Structure, corresponding to recluster.{cleanup,alloc_hosts,generate_intlookups}.[]
    """

    def __init__(self, group, section, card):
        """
            Initialize recluster command from card section, describing single recluster command

            :param group(core.igroups.IGroup): group name
            :param section(str): section in recluster section (one of cleanup/alloc_hosts/generate_intlookups)
            :param card(core.card.node.CardNode): Card node from one of lists: recluster.clenaup/recluster.alloc_hosts/recluster.generate_intlookups
            :return None:
        """

        self.group = group
        self.section_name = section
        self.command_name = "%s.%s" % (self.section_name, card.id)
        self.prerequisites = card.prerequisites

        # transform command form card, evaluating {{some python code}}, e. g. replacing <./utils/aa.py {{self.name}}> by <./utils/aa.py MSK_WEB_BASE> for MSK_WEB_BASE
        jinja_env = {
            'group' : self.group,
        }
        if len(self.group.card.reqs.hosts.location.location) == 1:
            jinja_env['reserved_group'] = self.group.parent.get_group('%s_RESERVED' % (self.group.card.reqs.hosts.location.location[0].upper()))

        self.command_body = jinja2.Template(card.command).render(**jinja_env)

    def as_str(self):
        result = "%s:\n    command: %s" % (green_text(self.command_name), dblue_text(self.command_body))
        if len(self.prerequisites) > 0:
            result += "\n    prerequisites: %s" % (",".join(map(lambda x: green_text(x), self.prerequisites)))

        return result

    def run_command(self):
        retcode, out, err = gaux.aux_utils.run_command(shlex.split(self.command_body), cwd=MAIN_DIR, raise_failed=False)
        return retcode, out, err
