from copy import copy
import importlib
import inspect
import json
import logging
import pkgutil
from itertools import product
import re
import os
from pathlib import Path
from typing import Dict, List

from jinja2 import StrictUndefined, Environment, contextfilter

from .solomon_loader import SolomonLoader
from . import KIND2SOLOMON_KIND

try:
    from importlib import import_module
    import_module("cloud.mdb.solomon-charts.internal.lib.filters")
    ARCADIA_BUILD = True
except ImportError:
    ARCADIA_BUILD = False


ENTITIES = KIND2SOLOMON_KIND.keys()
MAX_FILENAME_LENGTH = 55

SERVICE_AREAS = ["_common"]


def save_file(ctx, target_dir: str, kind: str, env_name: str, area: str, name: str, val: str):
    logging.info("Saving file: %s " % name)
    dir_path = os.path.join(target_dir, "{}/{}/{}".format(env_name, kind, area))
    logging.info("Checking (and creting) out dir: %s", dir_path)
    if ctx.obj['dry_run']:
        logging.info("dry_run, exit")
        return
    Path(dir_path).mkdir(parents=True, exist_ok=True)
    with open(os.path.join(dir_path, "{}.json".format(name)), 'w') as fn:
        fn.write(val)
    logging.info("Saved: %s" % os.path.join(dir_path, "{}.json".format(name)))


def get_render_contexts(env_name, template, cfg):
    # Read id of entity (template)
    raw_id = re.findall(r"(?<=\"id\": \")[_<>a-z0-9- ]+(?=\",)", template)[0]
    logging.info("Getting render contexts: %s, %s", env_name, raw_id)
    # Find keywords which should be replaced by configs in solomon.json
    keywords = set(re.findall(r"(?!=\<\< *)[a-z0-9_]+(?= *\>\>)", raw_id))
    # Find keywords which are in .opts section
    opt_vars = (
        x.replace('_val', '')
        for x in keywords
        if x in cfg['opts']
        or x.replace('_val', '') in cfg['opts']
    )
    ov_values = []
    # Create list of found keywords from .opts
    for option_key in opt_vars:
        # get config by keyword
        option_val = cfg['opts'][option_key]
        # add values if the option it list in format [keyword, value]
        if isinstance(option_val, list):
            ov_values.append([[option_key, x] for x in option_val])
        # add values if the option is dict: [keyword, key, keyword_val, value]
        if isinstance(option_val, dict):
            ov_values.append([[option_key, x[0], option_key + "_val", x[1]] for x in option_val.items()])
    # Create list of environment specific options from .env_opts
    opt_vars = (x for x in keywords if x in cfg['env_opts'][env_name].keys())
    for option_key in opt_vars:
        #  get found keyword in the current environment
        option_val = cfg['env_opts'][env_name][option_key]
        # add values if the option it list in format [keyword, value]
        if isinstance(option_val, list):
            ov_values.append([[option_key, x] for x in option_val])
        elif isinstance(option_val, dict):
            ov_values.append([[option_key, key, option_key + "_val", val] for key, val in option_val.items()])
    result = []

    # Create all possible combinations of values of found keywords in id of the entity
    opt_ctxs = product(*ov_values)
    for oc in opt_ctxs:
        # Create a copy of dict of .envs.env_name to render context
        ctx = copy(cfg['envs'][env_name])
        for t in oc:
            # Add item from opt_list to render context as keyword:list_item. Value from found list
            if len(t) == 2:
                ctx[t[0]] = t[1]
            # Add 2 items from opt_list to render context as keyword:dict_key and keyword_val:dict_val.
            elif len(t) == 4:
                ctx[t[0]] = t[1]
                ctx[t[2]] = t[3]
            # Adding items from db_ctx for keyword "db"
            if t[0] == "db" or t[0] == "sharded_db":
                ctx["db_ctx"] = cfg["db_ctxs"][t[1]]
        # Adding items from .db_ctx for as a "g"eneric context, availabe for all templates
        ctx["g"] = {"db_ctxs": cfg["db_ctxs"], "env_ctx": {}}
        # Adding items from .common_ctx: available for all templates
        for k in cfg["common_ctx"].keys():
            ctx["g"][k] = cfg["common_ctx"][k]
        # Adding items from .env_opts: available for all templates
        for k, v in cfg["env_opts"][env_name].items():
            ctx["g"]["env_ctx"][k] = v
        result.append(ctx)
    return result


def render_entity(template: str, ctx, j_env) -> str:
    return j_env.from_string(template).render(**ctx)


def create_entities(ctx, templates: List, env_name: str, target_dir: str, area: str, cfg: Dict, j_env):
    for t in templates:
        file_path = t["path"]
        logging.info("Creating entities for template: {}".format(file_path))
        with open(file_path) as file_:
            content = file_.read()
        rctxs = get_render_contexts(env_name, content, cfg)
        for rctx in rctxs:
            entity = render_entity(content, rctx, j_env)
            try:
                file_name = json.loads(entity)["id"]
                if len(file_name) > MAX_FILENAME_LENGTH:
                    raise Exception(f"id `{file_name}` is too long, max {MAX_FILENAME_LENGTH}")
            except Exception as e:
                logging.error("Error: %s", str(e))
                logging.error("Invalid entity:\n%s", entity)
                raise
            save_file(ctx, target_dir, t["kind"], env_name, t["area"], file_name, entity)


def get_areas(templates_dir: str, kinds: List[str], area: str):
    logging.info("Getting areas: %s, %s, %s", templates_dir, kinds, area)
    if kinds is None or len(kinds) == 0:
        kinds = ENTITIES
    else:
        kinds = list(set(kinds) & set(ENTITIES))

    if area is not None:
        return [{"area": area, "kind": k} for k in kinds if os.path.isdir(os.path.join(templates_dir, k, area))]
    result = []
    for kind in kinds:
        e_path = os.path.join(templates_dir, kind)
        if not os.path.exists(e_path):
            continue
        areas = [d for d in os.listdir(e_path) if os.path.isdir(os.path.join(e_path, d)) and d not in SERVICE_AREAS]
        for a in areas:
            result.append({"kind": kind, "area": a})
            logging.info("Adding kind %s, area %s", kind, a)
    return result


def filter_files(path, files, env_name):
    all_valid_files = list(filter(lambda name: name.endswith('.j2'), files))
    if "index.json" not in files:
        return all_valid_files

    with open(os.path.join(path, "index.json")) as file_stream:
        idx = json.load(file_stream)
    template_files_list = []
    if "include" in idx.keys():
        if len(idx["include"]) == 0:
            raise RuntimeError('Include statement in index.json cannot be empty if present')
        for file_spec in idx["include"]:
            f_env_name = file_spec["env_name"]
            f_files = file_spec["files"]
            if f_env_name not in env_name:
                continue
            if f_files == "*":
                template_files_list += all_valid_files
            elif isinstance(f_files, list):
                template_files_list += set(files).intersection(set(f_files))
    if "exclude" in idx.keys():
        exclude_list = []
        # if there wasnt any include statement, include all
        if not template_files_list:
            template_files_list = all_valid_files
        for file_spec in idx["exclude"]:
            f_env_name = file_spec["env_name"]
            f_files = file_spec["files"]
            if f_env_name not in env_name:
                continue
            if f_files == "*":
                exclude_list = all_valid_files
            elif isinstance(f_files, list):
                exclude_list = f_files
        template_files_list = list(set(template_files_list) - set(exclude_list))
    return template_files_list


def get_files(env_name, templates_dir, kind, area):
    if "YATEST" in os.environ.keys() and os.environ["YATEST"] == "1":
        from yatest.common import source_path
        logging.info("Source path: %s" % source_path(templates_dir))
        path = os.path.join(source_path(templates_dir), kind, area)
    else:
        path = os.path.join(templates_dir, kind, area)

    return filter_files(path, os.listdir(path), env_name)


def get_templates(env_name, templates_dir, kinds: List[str], area: str = None):
    logging.info("Getting templates %s, %s, %s", templates_dir, kinds, area)
    areas = get_areas(templates_dir, kinds, area)
    result = []
    for a in areas:
        logging.info("Loading for area %s", a)
        files = get_files(env_name, templates_dir, a["kind"], a["area"])
        result += [{"area": a["area"], "kind": a["kind"], "file": f,
                    "path": os.path.join(templates_dir, a["kind"], a["area"], f)
                    } for f in files if f.endswith(".j2")]
    logging.info("Loaded %d templates" % len(result))
    return result


def list_submodules(list_name, package_name):
    logging.info("Loading package: %s" % package_name.__path__)

    for loader, module_name, is_pkg in pkgutil.walk_packages(package_name.__path__, package_name.__name__+'.'):
        list_name.append(module_name)
        module_name = __import__(module_name, fromlist='dummylist')
        if is_pkg:
            list_submodules(list_name, module_name)


def m_prefix():
    if "YATEST" in os.environ.keys() and os.environ["YATEST"] == "1" or ARCADIA_BUILD:
        return "cloud.mdb.solomon-charts.internal.lib.filters.filter_"
    else:
        return "internal.lib.filters.filter_"


def module():
    if "YATEST" in os.environ.keys() and os.environ["YATEST"] == "1" or ARCADIA_BUILD:
        return "cloud.mdb.solomon-charts.internal.lib.filters"
    else:
        return "internal.lib.filters"


def load_filters_modules():
    all_modules = []
    list_submodules(all_modules, __import__(module(), fromlist='dummylist'))
    return [x for x in all_modules if x.startswith(m_prefix())]


def module_filters(module_name):
    module = importlib.import_module(module_name)
    functions = inspect.getmembers(module, inspect.isfunction)
    return [x for x in functions if not x[0].startswith("_")]


def resolve_var(context, value):
    return context[value]


def load_filters(env):
    logging.info("Loading filters")
    modules = load_filters_modules()
    logging.info("Loaded modules: %d" % len(modules))
    for m in modules:
        filters = module_filters(m)
        prefix = m[len(m_prefix()):]
        for f in filters:
            logging.info("Loading filter %s", ("%s_%s" % (prefix, f[0])))
            env.filters[("%s_%s" % (prefix, f[0]))] = f[1]
    env.filters["resolve"] = contextfilter(resolve_var)


def render(ctx, s: str, t: str, env_name: str, cfg: Dict, kinds: List[str], area: str):
    ts = get_templates(env_name, s, kinds, area)
    j_env = Environment(loader=SolomonLoader(s), undefined=StrictUndefined,
                        variable_start_string='<<', variable_end_string='>>')
    load_filters(j_env)
    create_entities(ctx, ts, env_name, t, area, cfg, j_env)
