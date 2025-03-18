# encoding: utf-8
import argparse
import io
import json
import logging
import os
import shutil
import six
import sys
import tarfile
import tempfile
import typing as tp  # noqa
import yaml

from library.python import fs
from mkdocs import config
from mkdocs.commands import build as mkdocs_build

from mkdocs_yandex import on_files as files_handler

from . import process
from . import util

logger = logging.getLogger(__name__)


# TODO replace with plugin
def add_single_page_md_to_docs(
        docs_dir,  # type: six.string_types
        cfg_dict,  # type: tp.Dict[six.string_types,tp.Any]
        ):
    # type: (...) -> None
    """
    Add single.md file and patch configuration
    :param docs_dir: full path to docs directory
    :param cfg_dict: dictionary with mkdocs configuration
    :return:
    """
    logger.info('Preparing single page version')

    logger.info(cfg_dict['plugins'])
    need_anchor = not util.is_yfm_enabled(cfg_dict, 'yfm20')
    logger.info(need_anchor)

    x = u'\n'.join(util.merge_content(docs_dir, cfg_dict['nav'], [], [], need_anchor))

    logger.debug('Rendering single page content')
    template = process.Preprocessor(docs_dir, util.is_yfm_enabled(cfg_dict, 'yfm20')).template(x)
    content = template.render(**cfg_dict['extra'])
    content = process.preprocess_markdown_for_single_page('', content, need_anchor)

    with io.open(os.path.join(docs_dir, 'single.md'), 'w', encoding='utf-8') as f:
        f.write(content.encode('utf-8'))

    cfg_dict['extra'].update({'single': True})
    cfg_dict['nav'] += [
        {u'Одностраничная версия': 'single.md'}
    ]


def add_single_page_to_output(
        docs_dir,  # type: six.string_types
        output_dir,  # type: six.string_types
        cfg,  # type: mkdocs.config.Config
        ):
    # type: (...) -> None
    """
    Build singlepage version of docs_dir and copy result to cfg['site_dir']/single
    :param docs_dir: full path to docs directory
    :param cfg: mkdocs configuration
    :return:
    """
    logger.info('Building single page version')

    x = u'\n'.join(util.merge_content(docs_dir, cfg.data['nav'], [], []))

    logger.debug('Rendering single page content')
    template = process.Preprocessor(docs_dir, util.is_yfm_enabled(cfg.data, 'yfm20')).template(x)
    content = template.render(**cfg.data['extra'])
    content = process.preprocess_markdown_for_single_page('', content)

    with util.temp_dir('single') as src_dir, util.temp_dir('single_result') as dst_dir:
        with io.open(os.path.join(src_dir, 'single.md'), 'w', encoding='utf-8') as f:
            f.write(content)
        cfg.load_dict({
            'docs_dir': src_dir,
            'site_dir': dst_dir,
            'nav': [
                {u'Документация': 'single.md'}
            ]
        })
        cfg.data['extra']['single'] = True

        mkdocs_build.build(cfg)

        shutil.copytree(
            os.path.join(dst_dir, 'single'),
            os.path.join(output_dir, 'single')
        )


def build_docs(
        docs_dir,  # type: six.string_types
        output_dir,  # type: six.string_types
        config_path,  # type: six.string_types
        single_version,  # type: bool
        variables,  # type: dict
        dependencies,  # type: dict
        preprocess_md_only,  # type: bool
        skip_preprocess=False,  # type: bool
        sandbox_dir=None,  # type: six.string_types
        ):
    # type: (...) -> None
    """
    Build docs using config file
    :param docs_dir: full path to directory with md files
    :param output_dir: full path to directory with html files
    :param config_path: full path to configuration yml file
    """
    logger.info('Building docs from %s to %s', docs_dir, output_dir)
    assert not (preprocess_md_only and skip_preprocess)

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    _, templated_config_path = tempfile.mkstemp()
    process.Preprocessor(docs_dir, False).preprocess_markdown(config_path, variables, output_path=templated_config_path)
    logger.debug('Using preprocessed config file %s', templated_config_path)

    with util.temp_dir() as docs_processing_tmp_dir:
        logger.debug('Using temp docs dir %s', docs_processing_tmp_dir)

        shutil.rmtree(docs_processing_tmp_dir)
        shutil.copytree(docs_dir, docs_processing_tmp_dir,
                        ignore=lambda src, names: filter(None, [n if os.path.islink(os.path.join(src, n)) else None for n in names]))

        util.fix_permissions(docs_processing_tmp_dir)

        cfg_dict = process.yaml_load(templated_config_path)
        cfg_dict['plugins'] = ['arcadium_helper'] + cfg_dict.get('plugins', [])
        cfg_dict.update(
            config_file=templated_config_path,
            docs_dir=docs_processing_tmp_dir,
            site_dir=output_dir,
        )

        # need to stash extra options because load_config() reuses global config_options.SubConfig object,
        # thus, leading to leakage of extra section between different Config objects
        extra = cfg_dict.pop('extra', {})
        extra.update(variables)

        cfg = config.load_config(**cfg_dict)

        # restore, explicitly initializing new SubConfig
        cfg.data['extra'] = config.config_options.SubConfig()
        cfg.data['extra'].update(extra)

        if not util.is_yfm_enabled(cfg.data, 'separate_nav'):
            _, files = files_handler.get_files(cfg)

            for file in files:
                os.remove(file.abs_src_path)

        replace_custom_homepage(cfg, docs_dir)

        if sandbox_dir:
            includes_target_dir = output_dir if preprocess_md_only else docs_processing_tmp_dir
            for s in os.listdir(sandbox_dir):
                fs.copy_tree(os.path.join(sandbox_dir, s), os.path.join(includes_target_dir, s))
            util.fix_permissions(includes_target_dir)

        if not skip_preprocess and (not util.is_yfm_enabled(cfg.data) or preprocess_md_only):
            renderer = process.Preprocessor(docs_processing_tmp_dir, util.is_yfm_enabled(cfg.data, 'yfm20'))
            for root, dirs, files in os.walk(docs_processing_tmp_dir):
                for f in files:
                    if os.path.splitext(f)[1] in ['.md', '.yml'] and not f.lower() == 'readme.md':
                        abs_f = os.path.join(root, f)
                        renderer.preprocess_markdown(abs_f, extra,
                                                     os.path.join(output_dir, os.path.relpath(abs_f, docs_processing_tmp_dir))
                                                     if preprocess_md_only else None)

        if preprocess_md_only:
            util.ensure_dir(output_dir)

            with open(os.path.join(output_dir, '__vars.json__'), 'w') as f:
                f.write(json.dumps(variables))
            shutil.copy(templated_config_path, os.path.join(output_dir, '__config.yml__'))

            return

        need_single_page = (cfg.data['extra'].get('single_page', False) and single_version is None) or single_version
        single_page_as_subdoc = 'single_page_as_subdoc' in cfg.data['theme'] and cfg.data['theme']['single_page_as_subdoc']

        # note that 'yandex' plugin can also build single page version if configured so
        if need_single_page and not single_page_as_subdoc:
            add_single_page_md_to_docs(docs_processing_tmp_dir, cfg.data)

        add_hidden_paths(cfg)

        deps_as_includes = cfg.data['extra'].get('peerdirs_as_includes', False)

        for kv in six.iteritems(dependencies):
            dest = kv[0]
            if deps_as_includes:
                open(os.path.join(docs_processing_tmp_dir, dest.split('/')[-1] + '.md'), 'w').close()
            else:
                with tarfile.open(kv[1], 'r:gz') as tar_file:
                    tar_file.extractall(os.path.join(docs_processing_tmp_dir, dest))

        mkdocs_build.build(cfg)

        if need_single_page and single_page_as_subdoc:
            add_single_page_to_output(docs_processing_tmp_dir, output_dir, cfg)

        cleanup_output(output_dir)

        if deps_as_includes:
            for kv in six.iteritems(dependencies):
                dep_name = kv[0].split('/')[-1]

                with tarfile.open(kv[1], 'r:gz') as tar_file, util.temp_dir(dep_name) as extract_dest:
                    tar_file.extractall(extract_dest)

                    dep_config_path = os.path.join(extract_dest, '__config.yml__')
                    dep_vars_path = os.path.join(extract_dest, '__vars.json__')
                    with io.open(dep_vars_path, 'r', encoding='utf-8') as f:
                        dep_vars = json.loads(f.read()) or {}

                    build_docs(docs_dir=extract_dest, output_dir=os.path.join(output_dir, dep_name),
                               config_path=dep_config_path, variables=dep_vars, preprocess_md_only=False,
                               single_version=single_version, dependencies={}, skip_preprocess=True,
                               )

        build_redirects(docs_dir, output_dir, deps_as_includes, dependencies)


def replace_custom_homepage(config, source_dir):
    custom = config.data['extra'].get('custom_homepage')
    if not custom:
        return

    custom_abspath = os.path.join(config['docs_dir'], custom)
    homepage_abspath = os.path.join(config['docs_dir'], 'index.md')

    if not os.path.exists(custom_abspath):
        raise Exception('Custom homepage {} does not exist in docs dir {}'.format(custom, source_dir))

    if os.path.exists(homepage_abspath):
        raise Exception('Can not replace existing homepage index.md with custom {}'.format(custom))

    os.rename(custom_abspath, homepage_abspath)

    for i, nav_item in enumerate(config['nav']):
        if isinstance(nav_item, dict):
            kv = next(iter(nav_item.items()))
            title, page = kv[0], kv[1]
        else:
            page, title = nav_item, None

        if page == custom:
            config['nav'][i] = 'index.md' if title is None else {title: 'index.md'}
            logger.debug('Replaced homepage with custom %s', custom)
            break


def add_hidden_paths(config):
    for hidden_path in config['extra'].get('add_hidden', []):
        hidden_abspath = os.path.join(config['docs_dir'], hidden_path)

        util.ensure_dir(os.path.dirname(hidden_abspath))
        io.open(hidden_abspath, 'w', encoding='utf-8').close()

        config['nav'].append({'hidden': hidden_path})
        logger.debug('Added hidden path for %s', hidden_path)


def cleanup_output(output_dir):
    for name in ['ya.make', 'redirects.txt', '__config.yml__', '__vars.json__']:
        shutil.rmtree(os.path.join(output_dir, name), ignore_errors=True)


def build_redirects(
        docs_dir,  # type: six.string_types
        output_dir,  # type: six.string_types
        deps_as_includes,
        dependencies,
        ):
    # type: (...) -> None
    """
    Create redirect rules for some links;
    Useful when you move or rename md files
    :param docs_dir: full path to directory with md files
    :param output_dir: full path to directory with html files
    """
    redirects = os.path.join(docs_dir, 'redirects.txt')
    if not os.path.exists(redirects):
        return

    deps_fragment = '(' + '|'.join([d.split('/')[-1] for d in dependencies]) + ')' if deps_as_includes else ''

    rewrites = []
    with io.open(redirects, 'r', encoding='utf-8') as f:
        for line in f:
            from_path, to_path = line.split(' ', 1)
            from_path = '^/docs/' + deps_fragment + from_path.replace('.md', '/?') + '$'
            to_path = '/docs/$1/' + to_path.replace('.md', '/')
            rewrites.append(' '.join(['rewrite', from_path, to_path, 'permanent;']))

    with io.open(os.path.join(output_dir, 'redirects.conf'), 'w', encoding='utf-8') as f:
        f.write(u'\n'.join(rewrites))


def build_tar(
        output_dir,  # type: six.string_types
        tar_path  # type: six.string_types
        ):
    # type: (...) -> None
    """
    Build tar from html files
    :param output_dir: full path to directory with html files
    :param tar_path: full path to tar archive
    :return:
    """
    if not tar_path:
        return

    def fix_permissions(tarinfo):
        tarinfo.mode = 0o644
        return tarinfo

    util.ensure_dir(os.path.dirname(tar_path))
    with tarfile.open(tar_path, 'w:gz') as tar_file:
        for root, dirs, files in os.walk(output_dir):
            for f in files:
                abs_path = os.path.join(root, f)
                if os.path.splitext(f)[-1] == 'html':
                    process.postprocess_html(abs_path)
                tar_file.add(abs_path, arcname=abs_path.replace(output_dir, ""), filter=fix_permissions)


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('--config', type=os.path.abspath, help="Path to configuration file")
    arg_parser.add_argument('--docs-dir', type=os.path.abspath, help="Path to directory with docs")
    arg_parser.add_argument('--sandbox-dir', type=os.path.abspath, help="Path to directory with files to include")
    arg_parser.add_argument('--output-dir', type=os.path.abspath, help="Path to directory with result htmls")
    arg_parser.add_argument('--output-tar', type=os.path.abspath, help="Path to archive with result htmls")
    arg_parser.add_argument('--add-single-version', dest='single_version', action='store_true', default=None, help='Build single page version')
    arg_parser.add_argument('--skip-single-version', dest='single_version', action='store_false', default=None, help='Do not build single page version')

    arg_parser.add_argument('--verbose', action='store_true', help='Show debug output')
    arg_parser.add_argument('--use-tmpdir', action='store_true', default=False, help='Use tempdirs as working dir')

    arg_parser.add_argument('--var', type=str, dest='vars', action='append', default=[], help='Additional variable to pass to configuration')
    arg_parser.add_argument('--dep', type=os.path.abspath, dest='deps', action='append', default=[], help='Path to directory with dependent book source files')
    arg_parser.add_argument('--preprocess-md-only', action='store_true', default=False, help='Preprocess and exit')

    args = arg_parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        stream=sys.stderr
    )

    variables = {}
    for var_descr in args.vars:
        parts = var_descr.split('=')
        variables[parts[0].strip()] = True if len(parts) == 1 else parts[1].strip()

    dependencies = {}
    for dep_descr in args.deps:
        parts = dep_descr.split(':')
        assert len(parts) == 3, '--dep argument expected in form $dep_node_build_root$:$arcadia_root_relative_path$:$tar_name$, got {}'.format(dep_descr)
        dependencies[parts[1]] = os.path.join(*parts)

    env_patch = {}
    if not args.use_tmpdir:
        logger.debug('Using cwd %s as TMPDIR', os.getcwd())
        env_patch = {'TMPDIR': os.getcwd()}

    with util.custom_env(env_patch), util.temp_dir(os.path.basename(args.docs_dir)) as tmp_dir:
        output_dir = args.output_dir or tmp_dir

        build_docs(args.docs_dir, output_dir, args.config, args.single_version, variables, dependencies, args.preprocess_md_only, sandbox_dir=args.sandbox_dir)
        build_tar(output_dir, args.output_tar)
