import jinja2
import os
# from pkg_resources import require
# require('PyYAML')
# import yaml
import yaml.parser
import yaml.scanner

from core.db import CURDB
import file_node
from aux_utils import convert_yaml_dict_to_list_recurse
from dict_processor import recurse_apply_extended, recurse_apply_conditionals, \
    recurse_apply_inline_include, recurse_apply_include, recurse_check_same_nodes, recurse_add_fname_to_includes, \
    recurse_find_includes
from inode import INode
from gaux.aux_colortext import red_text


class TRootNode(INode):
    """
        Root node. On top level we have file nodes.
    """

    INTERNAL_KEYS = []

    def __init__(self, fnames, verbose=False):
        """
            In initialization we construct tree from input file. Sequence of actions:
                - recursively load yaml files, starting with <fname>;
                - process <_include> keywords in loaded structures;
                - expand config nodes (every ConfigTemplateNode converted into simple node with certain backend)

            :param fnames(list of str): list of template files to load
            :param verbose(bool): add verbose output
        """

        super(TRootNode, self).__init__([], "root", None, [], dict())

        # load yaml files, recursively process all includes
        ystore = [(name, _load_yaml_wrapped(name)) for name in fnames]
        while True:
            loaded_includes = set(name for name, _ in ystore)
            cur_includes = _find_all_includes(ystore)
            extra_includes = cur_includes - loaded_includes
            if extra_includes:
                for include in extra_includes:
                    ystore.append((include, _load_yaml_wrapped(include)))
            else:
                break
        verbose_print(ystore, "Yaml loaded", verbose)

        # unfold all extends
        ystore = sum(map(lambda (xname, xvalue): recurse_apply_extended(xname, xvalue), ystore), [])
        verbose_print(ystore, "Extended applied", verbose)

        # process _test sections
        ystore = map(lambda (yname, fcontent):
                     recurse_apply_conditionals([(yname, fcontent)], (yname, fcontent), [(yname, None)])[0], ystore)
        verbose_print(ystore, "Conditional sections applied", verbose)

        # unfold inline includes
        ystore = recurse_apply_inline_include(ystore, ystore)
        verbose_print(ystore, "Inline includes applied", verbose)

        # unfold multiline includes
        recurse_apply_include(ystore, ystore, [])
        while len(_find_all_includes(ystore)) > 0:
            recurse_apply_include(ystore, ystore, [])
        verbose_print(ystore, "Included applied", verbose)

        # check if we have nodes with same name and anchor
        recurse_check_same_nodes(ystore, [])

        # generate structure with nodes
        for fname, content in ystore:
            self.children.append(file_node.TFileNode(fname, content))

    def render(self, strict=True):
        return "Root:\n%s" % self.render_childs(strict=strict)


def _load_yaml_wrapped(config_name):
    """
        Caching wrapper of load from yaml function.
    """

    # can not cache files not from db directory
    apath = os.path.realpath(os.path.abspath(config_name))
    if not apath.startswith(CURDB.get_path()):
        result = _load_yaml(config_name)
    else:
        result = CURDB.cacher.try_load([config_name])
        if result is None:
            result = _load_yaml(config_name)
            CURDB.cacher.save([config_name], result)
            CURDB.cacher.update()

    # get rid of includes
    recurse_add_fname_to_includes(result, config_name)  # remove local includes

    return result


def _load_yaml(config_name):
    """
        In this function we construct python structures from yaml config.
        The followings steps are performed:
            - process jinja templates from content of config file (as text file)
            - load into yaml structures
            - convert [(key1, value1), (key2, value2), ...] into [ {key1 : value1}, {key2 : value2}, ...]
                for more comfortable post-processing
            - convert all includes to some default form

        :param config_name(str): - absolute path of config file
        :return (file_name, yaml.data) - pair with filename and data from yaml
    """

    if not os.path.exists(config_name):
        raise Exception("Config <%s> does not exists" % config_name)

    # load raw data
    data = open(config_name).read()

    # convert jinja2 templates
    try:
        t = jinja2.Template(data)
    except jinja2.exceptions.TemplateSyntaxError as e:
        s = ["Got error while parsing jinja templates:", "    File <%s:%s>, line <%s>" % (
            red_text(config_name), red_text(e.lineno), red_text(data.split('\n')[e.lineno - 1])),
             "    Jinja error <%s> with message <%s>" % (type(e), e.message)]
        raise Exception("\n".join(s))
    except Exception:
        print "Got exceptoin while parsing file <%s>" % config_name
        raise

    data = t.render().encode('utf8')

    # load yaml
    try:
        data = yaml.load(data)
    except (yaml.parser.ParserError, yaml.scanner.ScannerError) as e:
        print e
        print e.__dict__
        s = "Got error while parsing yaml file <%s>:\n%s" % (config_name, red_text(str(e)))
        raise Exception(s)

    # convert all dict from yaml into list of (key, value) tuples
    return convert_yaml_dict_to_list_recurse(data, [os.path.basename(config_name)])


def _find_all_includes(ystore):
    includes = []
    for fname, fdata in ystore:
        includes.extend(recurse_find_includes(fname, fdata))
    includes = set(map(lambda x: x[0][0], includes))
    return includes


def verbose_print(ystore, title, verbose):
    if not verbose:
        return

    import pprint
    pp = pprint.PrettyPrinter(indent=4)
    s = "============================ %s =============================" % title
    print s
    pp.pprint(ystore)
    print "=" * len(s)
