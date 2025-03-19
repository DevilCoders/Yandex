#!/usr/bin/env python3
'''mongo'''
# pylint: disable=broad-except

import logging
from datetime import datetime
from types import SimpleNamespace as ns

from lib import Conductor, Remote
from lib import exceptions as e
from lib import utils
from pymongo import MongoClient
from pymongo.errors import DuplicateKeyError

LOG = logging.getLogger()


class Mongo():
    """Mongo client"""

    def __init__(self):
        cfg = utils.load_config()
        url = "mongodb://{0[user]}:{0[pass]}@{0[hosts]}/{0[authdb]}".format(
            cfg.mongo.toDict()
        )
        if cfg.mongo.args:
            url = "{}?{}".format(url, cfg.mongo.args)

        self.client = MongoClient(
            url,
            connect=False,
            connectTimeoutMS=1000,
            socketTimeoutMS=1000,
            serverSelectionTimeoutMS=3000
        )
        LOG.debug(self.client)
        try:
            self.client.admin.command("ismaster")
            self.db = self.client['yab']           # pylint: disable=invalid-name
            self.resources = self.db['resources']
        except Exception as exc:
            LOG.error("Failed init MongoClient: %s", exc)
            raise

    def create_dom0(self, dom0):
        """Create physical host"""

        physical_fqdn = dom0.get_hostname()
        conductor_info = Conductor().host_info(physical_fqdn)
        res = Remote(physical_fqdn).get_resources()
        LOG.debug("Get resources %s: %s", physical_fqdn, res)
        if res is None:
            status = {
                'status': 500,
                'msg': 'resource not found: {}'.format(physical_fqdn)
            }
            LOG.error("Failed to get remote resources: %s", status)
            return status

        cpu_list = set(range(int(res["cpu"])))
        for container in list(dom0.get_containers()):
            cpu_list -= set(container['cpuList'])

        post = {
            "fqdn": physical_fqdn,
            "cpu": res["cpu"],
            "cpuAvailList": list(cpu_list),
            "ram": res["memory"],
            "containers": list(dom0.get_containers()),
            "datacenter": conductor_info["root_datacenter"],
            "diskType": res["storage"]["disks"],
            "diskSize": res["storage"]["size"],
            "pool": res["pool"],
            "project": conductor_info["project"],
            "status": "Running",
            "lastModified": datetime.now(),
            "tags": "production"  # need list of tag
        }
        LOG.debug("Insert document into mongodb: %s", post)
        try:
            self.resources.insert_one(post)
            status = {
                'status': 200,
                'msg': 'mongodb insert success for {}'.format(physical_fqdn)
            }
            LOG.info("Success create dom0 host %s: %s", physical_fqdn, status)
        except Exception as exc:
            status = {
                'status': 500,
                'msg': 'mongodb error for {}: {}'.format(physical_fqdn, exc)
            }
            LOG.exception("MongoClient exception for %s: %s", physical_fqdn, exc)

        return status

    def get_dom0(self, fqdn, list_all=False):
        '''
        check physical host. If exist, update timestamp and return stats
        if list_all not False, return info about all dom0 hosts

        return dict status = {
            "status": <http_code>,
            "result": <object>,
            "msg": <string>,
        }
        '''
        if list_all:
            LOG.debug("Query mongo about all dom0 hosts")
            physical_hosts = list(self.resources.find({}, {"_id": False}))
            return {
                'status': 200,
                'msg': "All hosts info",
                'result': physical_hosts,
            }

        LOG.debug("Query mongo about %s", fqdn)
        doc = self.resources.find_one({"fqdn": fqdn}, {"_id": False})
        LOG.debug("Mongo response about %s: %s", fqdn, doc)
        if doc is None:
            raise e.IceDom0Exception(
                status=404,
                detail='Host not found {}'.format(fqdn),
            )

        result = self.resources.update_one(
            {"fqdn": fqdn},
            {"$set": {"lastModified": datetime.now()}}
        )
        return {
            'status': 200,
            'msg': 'Host {} exists, touch it: {}'.format(fqdn, result.raw_result),
            'result': doc,
        }

    def dom0_pick(self, fqdn, args):
        """find apropriate dom0 host"""

        pipeline_project = {
            "fqdn": "$fqdn",
            "pool": "$pool",
            "freeCpu": {"$subtract": ["$cpu", {"$sum": "$containers.cpu"}]},
            "freeRam": {"$subtract": ["$ram", {"$sum": "$containers.ram"}]}
        }
        pipeline_match = {
            "pool": args.pool,
            "freeCpu": {"$gte": args.cpu},
            "freeRam": {"$gte": args.ram},
        }
        if fqdn:
            pipeline_match["fqdn"] = fqdn
        elif args.dc == 'any':
            pipeline_project["diskType"] = "$diskType"
            pipeline_match["diskType"] = args.diskType
        else:
            pipeline_project["diskType"] = "$diskType"
            pipeline_project["dc"] = "$datacenter"
            pipeline_match["diskType"] = args.diskType
            pipeline_match["dc"] = args.dc

        pipeline = [
            {"$project": pipeline_project},
            {"$match": {"$and": [pipeline_match]}},
            {"$sort": {"freeCpu": 1}},
            {"$limit": 5},
            {"$sort": {"freeRam": 1}},
            {"$limit": 1}
        ]
        LOG.debug("dom0_pick for %s, pipeline %s", args.fqdn, pipeline)
        hostname = None
        hosts = list(self.resources.aggregate(pipeline))
        if hosts:
            hostname = hosts[0].get("fqdn", None)

        if not hostname:
            LOG.error(
                "Failed to find host for %s by pipeline %s, mongo response: %s",
                args.fqdn, pipeline, hosts
            )
            raise e.IceDom0Exception(
                status=404,
                detail="Failed to pick dom0 for container {}".format(args.fqdn),
            )
        return hostname

    def dom0_add_container(self, dom0, args):
        """Update info about dom0 host"""

        physical_fqdn = dom0.get_hostname()
        try:
            result = self.resources.update_one(
                {"fqdn": physical_fqdn},
                {"$push": {"containers": {
                    "fqdn": args.fqdn,
                    "name": utils.name_from_fqdn(args.fqdn, kind="lxd"),
                    "status": "New",
                    "project": args.project,
                    "profiles": ["bootstrap"] + args.profiles,
                    "filesystem": args.filesystem,
                }}},
            )
        except DuplicateKeyError:
            raise e.IceCreateContainerException(
                detail="Container {} alredy exists".format(args.fqdn)
            )
        return result.raw_result

    def dom0_update(self, dom0):
        """Update info about dom0 host"""

        physical_fqdn = dom0.get_hostname()
        res = Remote(physical_fqdn).get_resources()
        LOG.debug("Host %s resources %s", physical_fqdn, res)

        self.resources.find_and_modify(
            query={"fqdn": physical_fqdn},
            update={"$set": {"containers": list(dom0.get_containers())}},
            upsert=True
        )
        pipeline = [
            {"$project": {
                "_id": 0,
                "fqdn": 1, "cpuAvailList": 1, "cpuList": "$containers.cpuList"
            }},
            {"$unwind": "$cpuList"},
            {"$unwind": "$cpuList"},
            {"$group": {
                "_id": {"fqdn": "$fqdn", "cpuAvailList": "$cpuAvailList"},
                "cpuList": {"$push": "$cpuList"},
            }},
            {"$project": {
                "_id": 0,
                "fqdn": "$_id.fqdn",
                "cpuAvailList": "$_id.cpuAvailList",
                "cpuList": 1,
            }},
            {"$match": {"fqdn": physical_fqdn}}
        ]

        LOG.debug("Update pipeline for %s: %s", physical_fqdn, pipeline)
        result = list(self.resources.aggregate(pipeline))
        LOG.debug("Update pipeline for %s result: %s", physical_fqdn, result)
        result = result[0] if result else {}
        result = ns(**{
            "cpuList": result.get("cpuList", []),
            "cpuAvailList": result.get("cpuAvailList", []),
        })
        cpu_avail_list = list(set(range(res["cpu"])) - set(result.cpuList))
        diff = set(result.cpuAvailList).symmetric_difference(set(cpu_avail_list))
        diff = [
            "{} {}".format("+" if c in cpu_avail_list else "-", c) for c in diff
        ]
        LOG.debug(
            "Update cpuAvailList for %s from %s to %s, diff: %s",
            physical_fqdn,
            result.cpuAvailList,
            cpu_avail_list,
            diff,
        )
        result = self.resources.update(
            {"fqdn": physical_fqdn},
            {"$set": {"cpuAvailList": cpu_avail_list}},
        )
        return result

    def cloud_info(self):
        """Collect cloud stats"""

        info = {
            "datacenters": [],
            "projects": [],
            "images": [],
            "profiles": [],
            "diskType": [],
            "pools": [],
            "filesystems": [],
        }
        datacenters = self.resources.find().distinct("datacenter")
        info["datacenters"] = datacenters

        projects = list(self.db["projects"].find({}, {"_id": 0, "project": 1}))
        LOG.debug("Got cloud projects %s", projects)
        if projects:
            info["projects"] = [p["project"] for p in projects]

        images = self.db["images"].find()
        if images:
            info["images"] = images[0]["images"]

        profiles = self.db["profiles"].find()
        if profiles:
            info["profiles"] = profiles[0]["profiles"]

        filesystems = self.db["filesystems"].find()
        if filesystems:
            info["filesystems"] = filesystems[0]["filesystems"]

        disk_type = self.resources.find().distinct("diskType")
        info["diskType"] = disk_type

        pools = list(self.db["pools"].find({}, {"_id": 0, "pool": 1}))
        LOG.debug("Got cloud pools %s", pools)
        if pools:
            info["pools"] = [p["pool"] for p in pools]

        LOG.info("Current cloud info: %s", info)

        return info

    def container_info(self, fqdn):
        """Get container info"""
        result = self.resources.find_one(
            {"containers.fqdn": fqdn},
            {"_id": 0, 'containers.$': 1, 'fqdn': 1}
        )
        container = {}
        if result:
            container = result['containers'][0]
            container["dom0"] = result["fqdn"]

        return container

    def container_set_status(self, fqdn, status):
        """Update container status"""
        result = self.resources.update(
            {"containers.fqdn": fqdn},
            {"$set": {"containers.$.status": status}}
        )

        return result

    def projects_info(self, project="all"):
        '''return info about all projects'''

        pipeline = [
            {"$project": {"containers": "$containers"}},
            {"$unwind": "$containers"},
            {"$group": {
                "_id": "$containers.project",
                "cpu": {"$sum": "$containers.cpu"},
                "ram": {"$sum": "$containers.ram"},
                "diskSize": {"$sum": "$containers.diskSize"}
            }}
        ]
        if project != "all":
            pipeline.insert(2, {"$match": {"containers.project": project}})

        prj_res = list(self.resources.aggregate(pipeline))
        LOG.debug("Projects pipeline %s result %s", pipeline, prj_res)

        projects_res = dict()
        try:
            projects = self.db['projects']
            projects_res = {p["project"]: p for p in list(projects.find({}, {"_id": 0}))}
            for prj in projects_res:
                projects_res[prj].update({"cpu": 0, "ram": 0, "diskSize": 0})

            for prj in prj_res:
                projects_res[prj["_id"]].update({
                    "cpu": prj["cpu"],
                    "ram": prj["ram"],
                    "diskSize": prj["diskSize"],
                })
        except Exception as exc:
            LOG.exception("Error while finding project info: %s", exc)

        return projects_res

    def users(self):
        '''Return users list'''
        auth_users = self.db['users'].find_one({})["users"]
        LOG.debug("Authorized users: %s", auth_users)
        return auth_users

    def project_to_pool(self, project):
        '''return pool name'''
        doc = self.db['pools'].find_one({"projects": project}, {"_id": False, "pool": 1})
        LOG.debug("Container pool: %s", doc['pool'])
        return doc['pool']
