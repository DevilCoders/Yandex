#!/usr/bin/env python
# coding=utf-8

import argparse
import datetime
import requests
import sys

from collections import defaultdict

import resources_counter
import startrek

from servers_order import HOST_OPERATION_TICKET, DEFAULT_ORDER_ASSIGNEE


def get_task_credit(task, logins_to_check):
    task = startrek.get_issue(task)

    credit_line_prefix = u"кредит "

    res = resources_counter.ResourcesCounter()

    for comment in task.comments:
        if not comment.createdBy or comment.createdBy.id not in logins_to_check:
            continue

        for full_line in comment.text.split("\n"):
            line = full_line.strip().lower().replace("==", "=")

            if not line.startswith(credit_line_prefix):
                continue

            line = line.split()
            if len(line) < 4:
                sys.stderr.write(("get_task_credit: credit line is too short %s:\n%s\n" % (
                    task.key, full_line.strip())).encode("utf-8"))
                continue

            assert line[0] == credit_line_prefix.strip()
            dc = line[1]
            credit_size = line[2]
            resource_name = line[3]

            if dc not in resources_counter.DC_LIST:
                sys.stderr.write(("get_task_credit: unknown dc %s at %s\n" % (dc, task.key)).encode("utf-8"))
                continue

            if resource_name not in [u"core", u"cores", u"ядро", u"ядра", u"ядер"]:
                sys.stderr.write((
                                     "get_task_credit: unknown credit resource %s at %s\n" % (
                                     resource_name, task.key)).encode("utf-8"))
                continue

            try:
                credit_size = float(credit_size)
            except:
                sys.stderr.write(
                    "get_task_credit: failed to parse resource '%s' at %s\n" % (credit_size.encode("utf-8"), task.key))
                continue

            resource = resources_counter.empty_resources()
            resource["cloud"] = "credits"
            resource["dc"] = dc
            resource["cpu"] = credit_size

            res.add_resource(resource)

    return res


def credits_check(update=False, cleanup=False, ping=False):
    config = resources_counter.load_config()

    main_task = startrek.get_issue(config["credits"]["task"])
    logins_to_check = config["credits"]["logins_to_check"]

    description = main_task.description

    auto_section_prefix = u"===Кредиты\n"
    prefix_description, _, credits_section = description.partition(auto_section_prefix)

    res_sections = defaultdict(list)

    resources_sum = resources_counter.ResourcesCounter()
    resources_table = {}

    # rtc_credits
    issues_filter = config["credits"]["filter"]
    credit_issues = [issue.key for issue in startrek.search_issues(search_filter=issues_filter)]

    sections_tasks = read_sections(credits_section)
    section_task_ids = set().union(*sections_tasks.values())
    sections_tasks['rtc_credits'] = [f for f in credit_issues if f not in section_task_ids]

    for section, tasks in sections_tasks.iteritems():
        for task in tasks:
            task = startrek.get_issue(task)

            if task.status.key == "closed":
                if cleanup and section == "Closed":
                    continue

                res_sections["Closed"].append(task.key + "\n")
                continue

            deadline = task.deadline
            if deadline:
                deadline = datetime.datetime.strptime(deadline, "%Y-%m-%d").date()

            resources = get_task_credit(task, logins_to_check=logins_to_check)

            resources_sum += resources
            resources_table[task.key] = resources

            resources_string = resources.format_chosen("dc", "cpu") if resources else "!!resources unknown!!"

            if deadline:
                task_line = "%s %s\n%s\n\n" % (deadline, task.key, resources_string)
                section = "Ready" if deadline <= datetime.date.today() else "Future"
            else:
                section = "no deadline"
                task_line = "%s\n%s\n\n" % (task.key, resources_string)

            res_sections[section].append(task_line)

    if update:
        out = [auto_section_prefix]
        for section, lines in sorted(res_sections.iteritems()):
            out.append("====%s\n" % section)
            out.extend(sorted(lines))

        new_description = prefix_description + "".join(out)

        if description != new_description:
            main_task = startrek.get_issue(config["credits"]["task"])  # reload to avoid race
            if main_task.description != description:
                sys.stderr.write("main task description is already changed. please retry\n")
                return

            main_task.update(description=new_description)

            unlink_absent_links(main_task, new_description)

    return resources_sum, resources_table


def read_sections(description):
    res = defaultdict(list)

    for section, blocks in read_blocks(description).iteritems():
        for block in blocks:
            res[section].extend(block)

    return res


def read_blocks(description):
    section = None
    blocks = defaultdict(list)
    temp = []
    for line in description.split("\n"):
        if "==" in line or not line.strip():
            if temp:
                blocks[section].append(temp)
                temp = []
        else:
            task = line.split()[-1].split("/")[-1]
            if "-" in task:
                temp.append(line.split()[-1].split("/")[-1])

        if line.startswith("=="):
            section = line.split("=")[-1]

    if temp:
        blocks[section].append(temp)

    return blocks


def hosts_operations_check(cleanup=False, ping=False):
    missed_ticket_message = u"""
Коллеги, кажется этот тикет потерялся.
    """.strip()

    main_tickets_prefix = ["RUNTIMECLOUD", "ITDC"]

    main_task = startrek.get_issue(HOST_OPERATION_TICKET)

    description = main_task.description

    blocks = read_blocks(description)

    ready_gleb_blocks = []
    ready_other_blocks = []
    unready_blocks = []

    for section, blocks in blocks.iteritems():
        for block in blocks:
            main_tickets = []
            secondary_tickets = []
            for ticket in block:
                if ticket.split("-")[0] in main_tickets_prefix:
                    main_tickets.append(ticket)
                else:
                    secondary_tickets.append(ticket)

            if not main_tickets:
                sys.stderr.write("block without main ticket %s\n" % block[0])
                continue

            ready = True
            for ticket in main_tickets:
                if startrek.get_issue(ticket).status.key != "closed":
                    ready = False

                    if ping:
                        ticket = startrek.get_issue(ticket)
                        age = (datetime.datetime.now() - datetime.datetime.strptime(ticket.updatedAt.split("T")[0],
                                                                                    '%Y-%m-%d')).days
                        if age > 7:
                            cc = [DEFAULT_ORDER_ASSIGNEE]
                            if ticket.assignee:
                                cc.append(ticket.assignee.id)
                            cc = sorted(set(cc))

                            ticket.comments.create(text=missed_ticket_message, summonees=cc)

            if ready:
                if cleanup and section == "Ready":
                    continue

                for_gleb = False
                for task in block:
                    if "YPRES" in task or "QLOUDRES" in task:
                        for_gleb = True
                        break

                if for_gleb:
                    ready_gleb_blocks.append(block)
                else:
                    ready_other_blocks.append(block)
            else:
                unready_blocks.append(block)

    new_description = []
    new_description.append("====Ready Gleb")
    new_description.extend(["\n".join(block) + "\n\n" for block in ready_gleb_blocks])

    new_description.append("====Ready other")
    new_description.extend(["\n".join(block) + "\n\n" for block in ready_other_blocks])

    new_description.append("====Not ready")
    new_description.extend(["\n".join(block) + "\n\n" for block in unready_blocks])

    new_description = "\n".join(new_description)

    if description != new_description:
        main_task = startrek.get_issue(HOST_OPERATION_TICKET)  # reload to avoid race
        if main_task.description != description:
            sys.stderr.write("main task description is already changed. please retry\n")
            return

        main_task.update(description=new_description)

    unlink_absent_links(main_task, new_description)


def unlink_absent_links(task, new_description):
    queue_to_unlink = ["RUNTIMECLOUD", "YPRES", "QLOUDRES", "GENCFG", "ITDC"]

    for link in task.links:
        link_task = link.object.key
        if link_task.split("-")[0] in queue_to_unlink and not link_task in new_description:
            sys.stderr.write("unlink %s\n" % link_task)
            link.delete()


def evaluate_credits(config, reload=False):
    return credits_check()


def evaluate_migrations(config=None, reload=False):
    resources_sum = resources_counter.ResourcesCounter()

    sources = [
        "https://yp-quota-distributor.n.yandex-team.ru/qloud/stats",
        "https://yp-quota-distributor.n.yandex-team.ru/gencfg/stats"
    ]

    for url in sources:
        response = requests.get(url, verify=False)
        if response.status_code != 200:
            raise Exception("source error at %s: %s\n\n" % (url, response.json()))

        data = response.json()

        total_cpu = 0

        for cloud, cloud_data in data["totalOpenMigrations"].iteritems():
            total_cpu += sum(cloud_data["cpu"].values())
            for resource, resource_data in cloud_data.iteritems():
                for dc, val in resource_data.iteritems():
                    temp = resources_counter.empty_resources()
                    temp["cloud"] = "migrations"
                    temp["segment"] = cloud
                    temp["dc"] = dc
                    temp[resource] = val

                    resources_sum.add_resource(temp)

    return resources_sum


def print_migrations():
    resources_sum = evaluate_migrations()

    print resources_sum.format()


def test():
    return


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--credits', action="store_true")
    parser.add_argument('--hosts', action="store_true")
    parser.add_argument('--migrations', action="store_true")

    parser.add_argument('--ping', action="store_true")
    parser.add_argument('--test', action="store_true")

    options = parser.parse_args()

    if options.credits:
        credits_check(update=True)
    if options.hosts:
        hosts_operations_check(ping=options.ping)
    if options.migrations:
        print_migrations()
    if options.test:
        test()


if __name__ == "__main__":
    main()
