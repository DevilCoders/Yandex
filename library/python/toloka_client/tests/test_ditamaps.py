import os.path
import pytest
import xml.etree.ElementTree as ET
from collections import Counter
from yatest.common import source_path


def tk_main_docs_ditamap():
    return source_path('library/python/toloka-kit/docs/toloka-kit.ditamap')


@pytest.fixture(name='tk_main_docs_ditamap')
def tk_main_docs_ditamap_fixture():
    return tk_main_docs_ditamap()


def tk_reference_ditamap():
    return source_path('library/python/toloka-kit/docs/reference/_reference.ditamap')


@pytest.fixture(name='tk_reference_ditamap')
def tk_reference_ditamap_fixture():
    return tk_reference_ditamap()


def ck_main_docs_ditamap():
    return source_path('library/python/crowd-kit/docs/crowd-kit.ditamap')


@pytest.fixture(name='ck_main_docs_ditamap')
def ck_main_docs_ditamap_fixture():
    return ck_main_docs_ditamap()


def ck_reference_ditamap():
    return source_path('library/python/crowd-kit/docs/reference/_reference.ditamap')


@pytest.fixture(name='ck_reference_ditamap')
def ck_reference_ditamap_fixture():
    return ck_reference_ditamap()


@pytest.fixture(
    params=[
        tk_main_docs_ditamap,
        tk_reference_ditamap,
        ck_main_docs_ditamap,
        ck_reference_ditamap
    ],
    scope='session',
)
def ditamap_md_files(request):
    request.param = request.param()
    root_node = ET.parse(request.param).getroot()
    md_files = [elem.attrib['href'] for elem in root_node.iter() if
                elem.tag == 'topicref' and elem.attrib['href'].endswith('.md')]
    return request.param, md_files


def test_mds_from_ditamap_exists(ditamap_md_files):
    md_base_path, md_files = ditamap_md_files
    md_base_path = os.path.dirname(md_base_path)
    for md_file in md_files:
        md_file_path = os.path.join(md_base_path, md_file)
        assert os.path.exists(md_file_path), md_file_path + ' was not found'


def test_unique_in_ditamap(ditamap_md_files):
    ditamap_path, md_files = ditamap_md_files
    duplicates = [item for item, count in Counter(md_files).items() if count > 1]
    assert len(md_files) == len(set(md_files)), f'Ditamap {ditamap_path} has duplicates: {duplicates}'


@pytest.mark.parametrize(
    ['main_docs_ditamap', 'reference_ditamap'],
    [
        ('tk_main_docs_ditamap', 'tk_reference_ditamap'),
        ('ck_main_docs_ditamap', 'ck_reference_ditamap')
    ]
)
def test_md_from_dir_in_ditamap(main_docs_ditamap, reference_ditamap, request):
    def get_mds_from_ditamap(ditamap):
        root_node = ET.parse(ditamap).getroot()
        mds_set = set()
        for elem in root_node.iter():
            if elem.tag == 'topicref' and elem.attrib['href'].endswith('.md'):
                href = elem.attrib['href']
                if '/' in href:
                    href = href.split('/')[-1]
                mds_set.add(href)
        return mds_set

    main_docs_ditamap = request.getfixturevalue(main_docs_ditamap)
    reference_ditamap = request.getfixturevalue(reference_ditamap)

    md_files_ditamap = get_mds_from_ditamap(reference_ditamap).union(get_mds_from_ditamap(main_docs_ditamap))
    reference_dir = os.path.dirname(reference_ditamap)
    md_files_directory = [file for file in os.listdir(reference_dir) if file.endswith('.md')]

    for md_file in md_files_directory:
        assert md_file in md_files_ditamap, f'Ditamaps {main_docs_ditamap}, {reference_ditamap} does not contain docs file {md_file}'
