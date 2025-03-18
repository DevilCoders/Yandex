#!/usr/bin/python
# -*- mode: python; coding: utf-8 -*-

"""Tests for library.infected_masks.infected_masks."""

import cPickle
import pickle

from yatest import common

from library.infected_masks import infected_masks


def MatcherFromFile(
    filename='infected_masks/sb_masks',
    maker=None,
    maker_factory=infected_masks.BaseMatcherMaker,
):
    if maker is None:
        maker = maker_factory()
    with open(common.data_path(filename)) as mask_file:
        for i, line in enumerate(mask_file):
            mask, data, _ = line.rstrip().split('\t')
            maker.AddMask(
                mask,
                data,
            )
    return maker.GetMatcher()


def SeparateMatches(matches):
    masks = frozenset(
        mask
        for mask, _ in matches
    )
    data = frozenset(
        item
        for _, matches in matches
        for item in matches
    )
    return masks, data


def do_test_is_infected(maker_factory):
    matcher = MatcherFromFile(maker_factory=maker_factory)

    assert matcher.IsInfectedUrl('http://sub.example0.com/1/foo')
    assert matcher.IsInfectedUrl('http://sub.example0.com/1/')
    assert not matcher.IsInfectedUrl('http://sub.example0.com/1')

    assert matcher.IsInfectedUrl('http://sub.example1.com/1/foo')
    assert matcher.IsInfectedUrl('http://sub.example1.com/1/')
    assert not matcher.IsInfectedUrl('http://sub.example1.com/1')

    assert matcher.IsInfectedUrl('http://foo.sub.example0.com/1/')
    assert matcher.IsInfectedUrl('http://foo.sub.example1.com/1/')

    assert not matcher.IsInfectedUrl('http://foo-sub.example0.com/1/')
    assert not matcher.IsInfectedUrl('http://foo-sub.example1.com/1/')

    assert matcher.IsInfectedUrl('http://sub.example0.com/nosub')
    assert matcher.IsInfectedUrl('http://sub.example1.com/nosub')

    assert matcher.IsInfectedUrl('http://sub.example0.com/nosub?haha=da')
    assert matcher.IsInfectedUrl('http://sub.example1.com/nosub?haha=da')

    assert not matcher.IsInfectedUrl('http://sub.example0.com/nosub/')
    assert not matcher.IsInfectedUrl('http://sub.example1.com/nosub/')

    assert not matcher.IsInfectedUrl('http://sub.example0.com/nosubfoo')
    assert not matcher.IsInfectedUrl('http://sub.example1.com/nosubfoo')

    assert not matcher.IsInfectedUrl('http://sub.example0.com/nosub/foo')
    assert not matcher.IsInfectedUrl('http://sub.example1.com/nosub/foo')


def test_base_is_infected():
    do_test_is_infected(
        maker_factory=infected_masks.BaseMatcherMaker,
    )


def test_matcher_is_infected():
    do_test_is_infected(
        maker_factory=infected_masks.MatcherMaker,
    )


def do_test_find_all(maker_factory):
    matcher = MatcherFromFile(maker_factory=maker_factory)

    actual = matcher.FindAll('http://sub.sub.example2.com/there/it/is?nocache=da')
    assert len(actual) == 3
    masks, data = SeparateMatches(actual)
    assert masks == {
        'sub.example2.com/',
        'sub.sub.example2.com/there/',
        'sub.sub.example2.com/there/it/is',
    }
    assert data == {
        'data-14', 'data-17', 'data-18',
    }

    actual = matcher.FindAll('http://sup.sub.example2.com/there/it/is?nocache=da')
    assert len(actual) == 2
    masks, data = SeparateMatches(actual)
    assert masks == {
        'sub.example2.com/',
        'sup.sub.example2.com/there/',
    }
    assert data == {
        'data-14', 'data-15', 'data-16',
    }


def test_base_find_all():
    do_test_find_all(
        maker_factory=infected_masks.BaseMatcherMaker,
    )


def test_matcher_find_all():
    do_test_find_all(
        maker_factory=infected_masks.MatcherMaker,
    )


def do_test_empty_data(maker_factory, empty_data):
    maker = maker_factory()
    maker.AddMask('sub2.sub.example.com/1/', empty_data)
    maker.AddMask('sub.example.com/1/2/', empty_data)
    matcher = maker.GetMatcher()

    assert matcher.IsInfectedUrl('http://sub2.sub.example.com/1/2/3')
    actual = matcher.FindAll('http://sub2.sub.example.com/1/2/3')
    assert len(actual) == 2
    masks, data = SeparateMatches(actual)
    assert masks == {
        'sub2.sub.example.com/1/',
        'sub.example.com/1/2/',
    }
    assert data == {empty_data}


def test_base_empty_data():
    do_test_empty_data(
        maker_factory=infected_masks.BaseMatcherMaker,
        empty_data='',
    )


def test_matcher_empty_data():
    do_test_empty_data(
        maker_factory=infected_masks.MatcherMaker,
        empty_data=None,
    )


def do_test_pickle(maker_factory, pickle):
    matcher_orig = MatcherFromFile(maker_factory=maker_factory)
    matcher_data = pickle.dumps(matcher_orig, pickle.HIGHEST_PROTOCOL)
    matcher = pickle.loads(matcher_data)

    actual = matcher.FindAll('http://sub.sub.example2.com/there/it/is?nocache=da')
    assert len(actual) == 3
    masks, data = SeparateMatches(actual)
    assert masks == {
        'sub.example2.com/',
        'sub.sub.example2.com/there/',
        'sub.sub.example2.com/there/it/is',
    }
    assert data == {
        'data-14', 'data-17', 'data-18',
    }


def test_base_pickle():
    do_test_pickle(
        maker_factory=infected_masks.BaseMatcherMaker,
        pickle=pickle,
    )


def test_matcher_pickle():
    do_test_pickle(
        maker_factory=infected_masks.MatcherMaker,
        pickle=pickle,
    )


def test_base_cpickle():
    do_test_pickle(
        maker_factory=infected_masks.BaseMatcherMaker,
        pickle=cPickle,
    )


def test_matcher_cpickle():
    do_test_pickle(
        maker_factory=infected_masks.MatcherMaker,
        pickle=cPickle,
    )
