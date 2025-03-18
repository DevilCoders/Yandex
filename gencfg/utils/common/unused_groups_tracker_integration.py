#!/skynet/python/bin/python
# -*- coding: utf8 -*-
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg
import ujson

import argparse
import json
import logging

import datetime
import dateutil.parser
import pytz
import collections

from core.db import CURDB
import startrek_client
import io
import re

from startrek_client import Startrek
from startrek_client.settings import VERSION_V2
import gaux.aux_dispenser
import gaux.aux_abc
import gaux.aux_staff
import gaux.aux_utils
from aux.aux_decorators import memoize

WEEKS_TO_SOLVE_ISSUE = 2
TRANSITION_CLOSE = "close"
TRANSITION_INVALID = "invalid"

class UnresolvedOwnersError(Exception):
    pass

class UnableToExtractGroupFromSummary(Exception):
    pass

def createStartrekClient():
    trackerToken = os.environ["TRACKER_TOKEN"]

    client = Startrek(
        api_version=VERSION_V2, useragent="RX-1499 automaton",
        base_url="https://st-api.yandex-team.ru", token=trackerToken
    )

    return client

def createIssueSummaryByGroup(group):
    return u"Удаление genfg группы {}".format(group.card.name)

def extractGroupNameFromSummary(s):
    class DummyGroup:
        def __init__(self):
            class DummyCard:
                def __init__(self):
                    self.name = "GROUP_NAME"
            self.card = DummyCard()

    extractRe = re.compile(
        createIssueSummaryByGroup(DummyGroup())
            .replace("GROUP_NAME", "\\b(.*)\\b")
    )
    mo = extractRe.match(s)
    if mo:
        return mo.group(1)
    raise UnableToExtractGroupFromSummary()

def splitOwners(owners):
    humans = list(filter(lambda x: not x.startswith("robot-"), owners))
    robots = list(filter(lambda x: x.startswith("robot-"), owners))
    return (humans, robots)

@memoize
def groupOwners(group):
    resolutionLog = io.StringIO()
    class ResolutionWrapper:
        def __init__(self, ioString):
            self.ioString = ioString

        def __getattr__(self, item):
            def debugWrapper(s):
                logging.debug(s)
                self.ioString.write(s.decode())
                self.ioString.write(u"\n")

            def warningWrapper(s):
                logging.warning(s)
                self.ioString.write(s.decode())
                self.ioString.write(u"\n")

            def errorWrapper(s):
                logging.error(s)
                self.ioString.write(s.decode())
                self.ioString.write(u"\n")

            if item == 'debug':
                return debugWrapper
            if item == 'warning':
                return warningWrapper
            if item == 'error':
                return errorWrapper

    rw = ResolutionWrapper(resolutionLog)

    rw.debug("Resolving owners for group {}".format(group.card.name))

    # this logic is used inside web ui for gencfg
    gencfgOwners = group.card.get('resolved_owners', []) or group.card.get('owners', [])
    if gencfgOwners:
        rw.debug("Got group {} owners from gencfg".format(group.card.name))
        humans, robots = splitOwners(gaux.aux_staff.unwrap_dpts(gencfgOwners))
        if humans:
            return humans, robots, rw.ioString.getvalue()
        rw.warning("There is no human owners for group {} in gencfg".format(group.card.name))

    rw.warning("Failed to get owners from gencfg for group {}".format(group.card.name))

    if not group.card.get('nanny') is None:
        nannyServices = map(lambda x: x.name, group.card.nanny.services)
        if nannyServices:
            rw.debug("Using nanny services as fallback: {}".format(",".join(nannyServices)))

            nannyLogins = list()
            nannyGroups = list()

            for nannyService in nannyServices:
                logins = list()
                groups = list()
                try:
                    serviceInfo = gaux.aux_utils.load_nanny_url("v2/services/%s/auth_attrs/" % nannyService)
                    owners = serviceInfo['content']['owners']
                    logins = owners['logins']
                    groups = owners['groups']
                    nannyLogins.extend(logins)
                    nannyGroups.extend(groups)
                except:
                    rw.error("Failed to retrieve service {} owners from nanny".format(nannyService))

            if nannyLogins:
                rw.debug("Got group {} owners from nany services: ".format(group.card.name, ",".join(nannyServices)))
                humans, robots = splitOwners(nannyLogins)
                if humans:
                    return humans, robots, rw.ioString.getvalue()
                rw.warning("There is no human owners in nanny services: ".format(",".join(nannyServices)))

            if nannyGroups:
                raise RuntimeError("Time to resolve groups: {}".format(",".join(nannyGroups)))

            rw.warning("Got empty result from nanny, group {}. Using dispenser.project_key as last resort".format(group.card.name))
        else:
            rw.warning("Empty nanny services list in group {}".format(group.card.name))
    else:
        rw.warning("Missing nanny info in group {} card".format(group.card.name))

    if group.card.dispenser.project_key:
        rw.debug("Using dispenser.project_key {} as last resort".format(group.card.dispenser.project_key))
        dispenser = gaux.aux_dispenser.Dispenser(os.environ["DISPENSER_TOKEN"])
        try:
            project = dispenser.projects(group.card.dispenser.project_key)
        except ValueError:
            rw.error("Failed to get project {} from dispenser".format(group.card.dispenser.project_key))
            raise UnresolvedOwnersError("Failed to get group {} related services".format(group.card.name))
        groups = project["responsibles"]["yandexGroups"]
        groups = list(
            map(
                lambda x: gaux.aux_abc.unwrap_abc_group(x),
                map(
                    lambda x: "abc:" + x,
                    groups
                )
            )
        )

        rw.debug("Got group {} owners from dispenser".format(group.card.name))
        humans, robots = splitOwners(project["responsibles"]["persons"] + groups)
        if humans:
            return humans, robots, rw.ioString.getvalue()
        rw.warning("There is no human owners for dispenser.project_key {}".format(dispenser.project_key_key))

    raise UnresolvedOwnersError("Failed to get group {} related services".format(group.card.name))

def createIssueDescriptionByGroup(group, createdAt):
    humans, robots, resolutionLog = groupOwners(group)

    if createdAt is None:
        createdAt = datetime.datetime.now()
    else:
        createdAt = dateutil.parser.parse(createdAt)

    # +1 to be on safe side (for example we ran script in evening)
    deadlineAt = createdAt + datetime.timedelta(days=7 * WEEKS_TO_SOLVE_ISSUE + 1)

    if not humans:
        logging.error("Failed to get human owners for group {}".format(group.card.name))
        raise UnresolvedOwnersError("There is no human owners for group {}".format(group.card.name))

    wikiTemplate = u"""\
Группа: {groupName}
Владельцы: {humanOwnersList}
По нашим данным группа {groupName} не имеет активных сервисов в Няне и потому через {weeksToSolveIssue} недели, {deadlineDate} группа будет автоматически удалена.
Если вас устраивает такое поведением можете игнорировать этот тикет.
Если вы считаете, что удаление нельзя делать, пожалуйста, нажмите нажмите кнопку "не удалять" в блоке смены статусов и опишите причину, почему группу нельзя удалять.
Если вы не хотите получать уведомления по этой группе можете перевести тикет в статус "можно делать".

<{{Debug
%%
Owners: {humanOwnersList}
RelatedRobots: {robotsOwnersList}

OwnersResolutionLog:
{resolutionLog}\

Group card:
{groupCardAsString}\
%%
}}>\
"""
    groupCardAsString = group.card.save_to_str(group.parent.SCHEME)
    return wikiTemplate.format(
        groupName=group.card.name,
        humanOwnersList=", ".join(humans),
        robotsOwnersList=", ".join(robots),
        resolutionLog=resolutionLog,
        weeksToSolveIssue=WEEKS_TO_SOLVE_ISSUE,
        deadlineDate=datetime.datetime.strftime(deadlineAt, "%Y-%m-%d"),
        groupCardAsString=groupCardAsString.decode('utf-8')
    )

def groupsToRemoveGenerator(nanny_dump):
    allMentionedGroups = getAllMentionedGroupsFromNannyDump(nanny_dump)
    allDynamic = CURDB.groups.get_group("ALL_DYNAMIC")
    for g in allDynamic.slaves:
        if "_DYNAMIC_YP_" in g.card.name:
            continue
        if not g.card.name in allMentionedGroups:
            yield g

def groupsToRemoveWithIssuesGenerator(nanny_dump, existingSubtasks):
    for g in groupsToRemoveGenerator(nanny_dump):
        # Not best solution, but should be acceptable for our problem space
        matchedIssues = list(
            filter(
                lambda x: createIssueSummaryByGroup(g) == x.summary,
                existingSubtasks
            )
        )
        yield (g, matchedIssues)

def updateChildrenIssue(client, issue, group):
    logging.debug("Updating children issue for {}".format(group.card.name))
    description = createIssueDescriptionByGroup(group, issue.createdAt)
    summary = createIssueSummaryByGroup(group)
    logging.debug(
        "summary: {}, description: {}".format(
            summary.encode('utf-8'),
            description.encode('utf-8')
        )
    )
    issue.update(description=description)

def getAllMentionedGroupsFromNannyDump(nanny_dump):
    allMentionedGroups = set()

    for root, dirs, files in os.walk(nanny_dump):
        for f in files:
            if not f.endswith("json"):
                continue
            logging.debug("Reading/parsing {}".format(f))
            with open(os.path.join(root, f)) as fi:
                j = json.load(fi)
                usedGroups = list()
                if j['runtime_attrs']['content']['instances']['chosen_type'] == "EXTENDED_GENCFG_GROUPS":
                    if j["current_state"]['content']['summary']['value'] == 'OFFLINE':
                        continue
                    for g in j['runtime_attrs']['content']['instances']['extended_gencfg_groups']['groups']:
                        usedGroups.append(g['name'])
                logging.debug("Got {} groups".format(len(usedGroups)))
                for g in usedGroups:
                    allMentionedGroups.add(g)

    logging.debug("{} groups loaded".format(len(allMentionedGroups)))
    return allMentionedGroups

def getSubtasks(issue):
    existingSubtasksSummaries = list()
    for link in issue.links:
        if link.type.id == "subtask" and link.direction == 'outward':
            existingSubtasksSummaries.append(link.object)
    return existingSubtasksSummaries

def createTickets(args):
    rootIssueName = args.root_issue

    client = createStartrekClient()

    NewIssueInfo = collections.namedtuple(
        'NewIssueInfo',
        ['group', 'summary', 'description', 'owners', 'firstComment']
    )

    createdTicketsCount = 0
    groupsWithNewTickets = list()
    groupsWithoutOwners = list()
    try:
        rootIssue = client.issues[rootIssueName]
        logging.debug("Root issue {} located".format(rootIssueName))
        existingSubtasks = getSubtasks(rootIssue)
        issuesToCreate = list()

        for group, matchedIssues in groupsToRemoveWithIssuesGenerator(args.nanny_dump, existingSubtasks):
            logging.debug("For group {} found {} tickets with matching summary".format(group.card.name, len(matchedIssues)))
            for issue in matchedIssues:
                logging.debug("issue: {}".format(repr(issue)))

            if matchedIssues:
                logging.debug("There is already ticket with group {} in summary".format(group.card.name))
                continue

            try:
                if args.tickets_count is not None:
                    if createdTicketsCount >= args.tickets_count:
                        logging.debug(
                            "Reached {} ticket(s) limit. Stopping".format(args.tickets_count)
                        )
                        break

                humans, robots, resolutionLog = groupOwners(group)
                deadlineAt = datetime.datetime.now() + datetime.timedelta(days=7 * WEEKS_TO_SOLVE_ISSUE + 1)
                firstComment = u"""\
По нашим данным группа {groupName} не имеет активных сервисов в Няне и потому через 2 недели, {deadlineDate} группа будет автоматически удалена.
Если вы считаете, что удаление нельзя делать, пожалуйста, нажмите нажмите кнопку "не удалять" в блоке смены статусов и опишите причину, почему группу нельзя удалять.
Если вы не хотите больше получать уведомления по этой группе можете перевести тикет в статус "можно делать".\
""".format(
    groupName=group.card.name,
    deadlineDate=datetime.datetime.strftime(deadlineAt, "%Y-%m-%d"),
    )

                summary = createIssueSummaryByGroup(group)
                description = createIssueDescriptionByGroup(group, createdAt=None)
                logging.debug(
                    "summary: {}, description: {}".format(
                        summary.encode('utf-8'),
                        description.encode('utf-8')
                    )
                )

                issuesToCreate.append(
                    NewIssueInfo(
                        group, summary, description,
                        humans,
                        firstComment
                    )
                )

                createdTicketsCount += 1
            except UnresolvedOwnersError as e:
                groupsWithoutOwners.append(group)
                logging.error(e.args[0])

        if args.dry_run:
            for newIssueInfo in issuesToCreate:
                print u"{groupName}\t{ownersList}".format(groupName=newIssueInfo.group.card.name, ownersList=",".join(newIssueInfo.owners))
        else:
            for newIssueInfo in issuesToCreate:
                issue = client.issues.create(
                    queue=rootIssue.queue.key,
                    type={'name': 'Task'},
                    description=newIssueInfo.description,
                    summary=newIssueInfo.summary,
                    parent=rootIssue
                )
                groupsWithNewTickets.append((newIssueInfo.group.card.name, issue))
                issue.comments.create(summonees=newIssueInfo.owners, text=newIssueInfo.firstComment)

            logging.debug("Created {} ticket(s)".format(createdTicketsCount))
            logging.debug("There are {} groups without resolved owners".format(len(groupsWithoutOwners)))

            commentIo = io.StringIO()
            commentIo.write(u"Created {} ticket(s)\n".format(createdTicketsCount))

            for groupName, issue in groupsWithNewTickets:
                commentIo.write(u"{} - {}\n".format(groupName, issue.key))

            if groupsWithoutOwners:
                commentIo.write(u"Groups without owners:\n".decode())
                for g in groupsWithoutOwners:
                    commentIo.write(g.card.name.decode())
                    commentIo.write(u"\n")
            rootIssue.comments.create(text=commentIo.getvalue())

    except startrek_client.exceptions.NotFound:
        logging.fatal("Failed to locate root issue {}".format(rootIssueName))

def inviteOwners(args):
    rootIssueName = args.root_issue

    client = createStartrekClient()

    try:
        rootIssue = client.issues[rootIssueName]
        logging.debug("Root issue {} located".format(rootIssueName))
        existingSubtasks = getSubtasks(rootIssue)
        groupsWithoutOwners = list()
        ticketsWithInvites = 0

        commentsToCreate = list()

        for g, matchedIssues in groupsToRemoveWithIssuesGenerator(args.nanny_dump, existingSubtasks):
            if matchedIssues:
                if len(matchedIssues) > 1:
                    logging.error("Somehow we got more than one issue for group {}".format(g.card.name))
                    continue
                logging.debug("Found ticket for group {} to invite owners".format(g.card.name))
            else:
                continue

            if matchedIssues[0].status.key != "open":
                logging.debug(u"Issue {} in status != open".format(matchedIssues[0].key))
                continue

            try:
                humans, robots, resolutionLog = groupOwners(g)

                if args.tickets_count is not None:
                    if ticketsWithInvites >= args.tickets_count:
                        logging.debug(
                            "Reached {} ticket(s) limit. Stopping".format(args.tickets_count)
                        )
                        break
                createdAt = dateutil.parser.parse(matchedIssues[0].createdAt)
                updatedAt = dateutil.parser.parse(matchedIssues[0].updatedAt)
                sinceLastUpdate = datetime.datetime.utcnow().replace(tzinfo=pytz.utc) - updatedAt
                deadlineAt = createdAt + datetime.timedelta(days=7 * WEEKS_TO_SOLVE_ISSUE + 1)
                if sinceLastUpdate > args.inactivity_delta or args.force_invite:
                    text=u"""\
Напоминаем, что {deadlineDate} группа {groupName} будет автоматически удалена.
Если вы считаете, что удаление нельзя делать, пожалуйста, нажмите нажмите кнопку "не удалять" в блоке смены статусов и опишите причину, почему группу нельзя удалять.
Если вы не хотите больше получать уведомления по этой группе можете перевести тикет в статус "можно делать".\
""".format(
    groupName=g.card.name,
    deadlineDate=datetime.datetime.strftime(deadlineAt, "%Y-%m-%d"),
    )

                    ticketsWithInvites += 1
                    commentsToCreate.append(
                        (matchedIssues[0], humans, text)
                    )
                else:
                    logging.debug("Not enough time has passed since last update to bother owners")

            except UnresolvedOwnersError as e:
                logging.error("Unable to get owners for group {}".format(g.card.name))
                groupsWithoutOwners.append(g)
                logging.error(e.args[0])
                pass

        logging.debug("inviting owners")
        for issue, humans, text in commentsToCreate:
            if args.dry_run:
                print u"{}\t{}".format(issue.key, issue.summary)
            else:
                issue.comments.create(
                    summonees=humans,
                    text=text
                )

    except startrek_client.exceptions.NotFound:
        logging.fatal("Failed to locate root issue {}".format(rootIssueName))

def listGroupsToDelete(args):
    groupsWithoutOwners = list()
    for g in groupsToRemoveGenerator(args.nanny_dump):
        try:
            humans, robots, resolutionLog = groupOwners(g)
            print "{groupName} - {humanOwnersList}".format(
                groupName=g.card.name,
                humanOwnersList=", ".join(humans)
            )
        except UnresolvedOwnersError as e:
            groupsWithoutOwners.append(g)
            logging.error(e.args[0])
            pass

def createArgParser():
    parser = argparse.ArgumentParser()
    subparser = parser.add_subparsers()

    # create tickets
    createTicketsParser = subparser.add_parser('create', help='create tickets')
    createTicketsParser.add_argument(
        "--nanny-dump", dest="nanny_dump",
        help="path to nanny-dump folder",
        required=True
    )
    createTicketsParser.add_argument(
        "--tickets-count", dest="tickets_count",
        type=int,
        help="limit how many tickets will be created"
    )
    createTicketsParser.add_argument(
        "--root-issue", dest="root_issue",
        help="specify root issue",
        default="TEST-61742"
    )
    createTicketsParser.add_argument(
        "--dry-run", dest="dry_run",
        help="don't create issues, print affected groups/onwers instead",
        action='store_true'
    )
    createTicketsParser.set_defaults(mode='create')

    # invite owners
    inviteOwnersParser = subparser.add_parser('invite', help='invite owners')
    inviteOwnersParser.add_argument(
        "--nanny-dump", dest="nanny_dump",
        help="path to nanny-dump folder",
        required=True
    )
    inviteOwnersParser.add_argument(
        "--force", dest="force_invite",
        help="invite no matter what",
        default=False, action="store_true"
    )
    inviteOwnersParser.add_argument(
        "--tickets-count", dest="tickets_count",
        type=int,
        help="limit in how many tickets invites will be send"
    )
    inviteOwnersParser.add_argument(
        "--root-issue", dest="root_issue",
        help="specify root issue",
        required=True
        #default="TEST-61742"
    )
    inviteOwnersParser.add_argument(
        "--inactivity-delta", dest="inactivity_delta",
        help="how many days should have passed since last update in issue",
        type=int, default=7
    )
    inviteOwnersParser.add_argument(
        "--dry-run", dest="dry_run",
        help="don't send comment, print affected issues instead",
        action='store_true'
    )
    inviteOwnersParser.set_defaults(mode='invite')

    # generateGroupsForRemoval
    listGroupsToDeleteParser = subparser.add_parser('list', help='invite owners')
    listGroupsToDeleteParser.add_argument(
        "--nanny-dump", dest="nanny_dump",
        help="path to nanny-dump folder",
        required=True
    )
    listGroupsToDeleteParser.set_defaults(inactivity_delta=datetime.timedelta(days=7))
    listGroupsToDeleteParser.set_defaults(mode='list')

    closeIssuesWithRemovedGroups = subparser.add_parser('close',
        help="close issues with absent groups (for example removed)"
    )
    closeIssuesWithRemovedGroups.add_argument(
        "--dry-run", dest="dry_run",
        help="don't send comment/close tickets, print affected issues instead",
        action='store_true'
    )
    closeIssuesWithRemovedGroups.add_argument(
        "--root-issue", dest="root_issue",
        help="specify root issue",
        default="TEST-61742"
    )
    closeIssuesWithRemovedGroups.set_defaults(mode='close_solved_issues')

    removeGroups = subparser.add_parser('remove',
        help="remove groups (issue status=Ready for dev)"
    )
    removeGroups.add_argument(
        "--root-issue", dest="root_issue",
        help="specify root issue",
        required=True
        #default="TEST-61742"
    )
    removeGroups.add_argument(
        "--groups-count", dest="groups_count",
        type=int,
        help="limit how many groups will be removed"
    )
    removeGroups.add_argument(
        "--dry-run", dest="dry_run",
        help="don't remove groups, print affected groups instead",
        action='store_true'
    )
    removeGroups.add_argument(
        "--disable-tracker", dest="disable_tracker",
        help="remove groups, but don't touch assosiated issues",
        action='store_true'
    )
    removeGroups.set_defaults(mode='remove')

    return parser

def removeGroups(args):
    rootIssueName = args.root_issue

    client = createStartrekClient()

    try:
        groupsToRemove = list()
        rootIssue = client.issues[rootIssueName]
        logging.debug("Root issue {} located".format(rootIssueName))

        existingSubtasks = getSubtasks(rootIssue)
        for issue in existingSubtasks:
            if issue.status.key in ["readyForDev", "open"]:
                if issue.status.key == "open":
                    createdAt = dateutil.parser.parse(issue.createdAt)
                    issueAge = datetime.datetime.utcnow().replace(tzinfo=pytz.utc) - createdAt
                    if issueAge < datetime.timedelta(days=7 * WEEKS_TO_SOLVE_ISSUE + 1):
                        logging.debug("Issue {} isn't old enough. Skipping")
                        continue
                try:
                    logging.debug(u"Observing issue {}".format(issue.key))
                    groupName = extractGroupNameFromSummary(issue.summary)
                    logging.debug(u"Extracted group name from summary {} -> {}".format(issue.summary, groupName))
                    if CURDB.groups.has_group(groupName):
                        try:
                            transition = issue.transitions[TRANSITION_CLOSE]
                            groupsToRemove.append((issue, transition, groupName))
                        except:
                            logging.exception("Failed to obtain target transition")
                    if args.groups_count is not None:
                        if len(groupsToRemove) >= args.groups_count:
                            logging.debug(
                                "Reached {} groups limit. Stopping".format(args.groups_count)
                            )
                            break

                except UnableToExtractGroupFromSummary:
                    logging.error(u"Unable to extract group name from summary {}".format(issue.summary))
        if args.dry_run:
            for issue, transition, groupName in groupsToRemove:
                print u"{}\t{}\t{}".format(issue.key, transition.id, groupName)
        else:
            unableToRemove = list()
            removed = list()

            for issue, transition, groupName in groupsToRemove:
                group = CURDB.groups.get_group(groupName)
                if not group.get_group_acceptors():
                    try:
                        logging.debug("Removing group {}".format(groupName))
                        CURDB.groups.remove_group(group, acceptor=None, recursive=False)
                        removed.append((issue, transition, groupName))
                    except:
                        logging.error("Failed to remove group {}".format(groupName))
                        unableToRemove.append(group)
            CURDB.update(smart=True)

            if not args.disable_tracker:
                for issue, transition, groupName in removed:
                    transition.execute(
                        comment=u"""Группа {groupName} удалена в рамках зачистки.
Закрываем тикет.""".format(groupName=groupName),
                        resolution='fixed'
                    )
            logging.info("Affected issues: {}".format(
                    ", ".join(
                        map(lambda x: x[0].key, removed)
                    )
                )
            )

    except startrek_client.exceptions.NotFound:
        logging.fatal("Failed to locate root issue {}".format(rootIssueName))

def closeSolvedIssues(args):
    rootIssueName = args.root_issue

    client = createStartrekClient()

    try:
        issuesToClose = list()
        rootIssue = client.issues[rootIssueName]
        logging.debug("Root issue {} located".format(rootIssueName))

        existingSubtasks = getSubtasks(rootIssue)
        for issue in existingSubtasks:
            if issue.status.key == "open":
                try:
                    logging.debug(u"Observing issue {}".format(issue.key))
                    groupName = extractGroupNameFromSummary(issue.summary)
                    logging.debug(u"Extracted group name from summary {} -> {}".format(issue.summary, groupName))
                    if not CURDB.groups.has_group(groupName):
                        issuesToClose.append((issue, groupName))

                except UnableToExtractGroupFromSummary:
                    logging.error(u"Unable to extract group name from summary {}".format(issue.summary))
        if args.dry_run:
            for issue, groupName in issuesToClose:
                print u"{}\t{}".format(issue.key, groupName)
        else:
            for issue, groupName in issuesToClose:
                transition = issue.transitions[TRANSITION_INVALID]
                transition.execute(
                    comment=u"""Группа {groupName} уже удалена вне этого тикета.
Закрываем тикет.""".format(groupName=groupName)
                )

    except startrek_client.exceptions.NotFound:
        logging.fatal("Failed to locate root issue {}".format(rootIssueName))

def main():
    #logging.basicConfig(level=logging.DEBUG, stream=logging.StreamHandler(  ))
    logging.basicConfig(
        level=logging.DEBUG,
        filename="debug.log",
    )
    console = logging.StreamHandler()
    console.setLevel(logging.DEBUG)
    console.setFormatter(logging.Formatter("%(levelname)s:%(name)s:%(message)s"))
    logging.getLogger('').addHandler(console)

    parser = createArgParser()
    args = parser.parse_args()

    if args.mode == "create":
        createTickets(args)
    elif args.mode == "invite":
        args.inactivity_delta = datetime.timedelta(days=args.inactivity_delta)
        inviteOwners(args)
    elif args.mode == "list":
        listGroupsToDelete(args)
    elif args.mode == "close_solved_issues":
        closeSolvedIssues(args)
    elif args.mode == "remove":
        removeGroups(args)

if __name__ == "__main__":
    main()
