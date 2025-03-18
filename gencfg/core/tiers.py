import compiler
import os
import sys
import traceback
from pkg_resources import require
import ujson
import json
import copy

require('PyYAML')
import yaml

from core.card.node import CardNode, Scheme, CardReaderExt, load_card_node, save_card_node
from gaux.aux_utils import indent

# TODO: remove
# from gaux.aux_utils import raise_extended_exception
def raise_extended_exception(src_exception, pattern):
    raise type(src_exception), type(src_exception)(
        pattern % str(src_exception)), sys.exc_info()[2]


class Tier(CardNode):
    def __repr__(self):
        return str(sorted(['{}-0000000000'.format(s) for s in self.shard_ids]))

    def __init__(self, db, contents):
        super(Tier, self).__init__()
        self.db = db

        self.reinit(contents)

    def reinit(self, contents):
        for child in contents:
            self.__dict__[child] = contents[child]
        self.init_shard_ids()

    def __hash__(self):
        return hash(self.name)

    def __getitem__(self, item):
        return self.__dict__[item]

    def __iter__(self):
        return self.__dict__.__iter__()

    def init_shard_ids(self):
        self.shard_ids = []
        self.shard_weights = []
        for primus in self.primuses:
            primus_name = primus['name']
            shards = primus['shards']
            if self.db.version >= '2.2.52':
                shards = range(primus['shards']['first'], primus['shards']['last'] + 1)
            for n in shards:
                if 'shardid_format' in self.properties and self.properties['shardid_format'] is not None:
                    shard_id = self.properties['shardid_format'] % {'primus_name': primus_name, 'shard_id': n}
                else:
                    shard_id = '%s-%03d' % (primus_name, n)
                self.shard_ids.append(shard_id)
                if 'weight' in primus:
                    self.shard_weights.append(primus['weight'])
                else:
                    self.shard_weights.append(1.0)
        self.shards_count = len(self.shard_ids)

    def __eq__(self, other):
        if not isinstance(other, Tier):
            return False
        return (self.name, self.primuses) == (other.name, other.primuses)

    def get_shard_id(self, n):
        return str(self.shard_ids[n])

    def get_shard_weight(self, n):
        return self.shard_weights[n]

    def get_shard_id_for_searcherlookup(self, n):
        postfix = '-0000000000'
        if 'shardid_format' in self.properties and self.properties['shardid_format'] is not None:
            postfix = ''

        return '%s%s' % (self.get_shard_id(n), postfix)

    def get_shards_count(self):
        return self.shards_count

    def get_primuses(self):
        return [primus['name'] for primus in self.primuses]

    def get_groups_and_intlookups(self):
        return self.db.tiers.get_tier_groups_and_intlookups(self)

    def update(self, f):
        indent = ' ' * 4
        print >> f, '-%sname: %s' % (indent[1:], self.name)
        print >> f, '%sdisk_size: %s' % (indent, self.disk_size)
        print >> f, '%sprimuses:' % indent
        for primus in self.primuses:
            print >> f, '%s-%s%s: %s' % (indent * 2, indent[1:], 'name', primus['name'])
            print >> f, '%s%s: [%s]' % (indent * 3, 'shards', ', '.join(str(x) for x in primus['shards']))

    def as_dict(self):
        return dict(
            name=self.name,
            description=self.description,
            disk_size=self.disk_size,
            primuses=self.primuses,
            properties=self.properties
        )


class Tiers(object):
    def __init__(self, db):
        self.db = db

        if self.db.version >= '2.2.52':
            self.TIERS_FILE = os.path.join(self.db.PATH, 'tiers.json')
            contents = ujson.loads(open(self.TIERS_FILE).read())
        else:
            self.TIERS_FILE = os.path.join(self.db.PATH, 'tiers.yaml')
            contents = yaml.load(open(self.TIERS_FILE).read())

        self.tiers = {}
        for value in contents:
            tier = Tier(self.db, value)
            if tier.name in self.tiers:
                raise Exception('Two occurences of the same tier %s' % tier.name)
            self.tiers[tier.name] = tier

    def check_tier(self, tier):
        if tier not in self.tiers:
            raise Exception("Tier '%s' does not exist." % tier)

    def has_tier(self, tier):
        return tier in self.tiers

    def get_tier_names(self):
        return self.tiers.keys()

    def get_tiers(self):
        return self.tiers.values()

    def get_tier(self, name):
        self.check_tier(name)
        return self.tiers[name]

    def set_tier(self, tier):
        assert (isinstance(tier, Tier))
        self.tiers[tier.name] = tier

    def primus_shard_id(self, tier, n):
        self.check_tier(tier)
        return self.tiers[tier].get_shard_id(n)

    def primus_shard_id_for_searcherlookup(self, tier, n):
        self.check_tier(tier)
        return str(self.tiers[tier].get_shard_id_for_searcherlookup(n))

    def get_total_shards(self, tier):
        #        self.check_tier(tier)
        return self.tiers[tier].get_shards_count()

    def add_tier(self, tier):
        assert (not self.has_tier(tier.name))
        self.tiers[tier.name] = tier

    def add_tier_from_dict(self, d):
        def fix_shards_format(primus):
            return {
                'name': primus['name'],
                'weight': primus.get('weight', 1.0),
                'shards': {
                    'first': primus['shards'][0],
                    'last': primus['shards'][-1]
                }
            }

        new_tier_card = copy.copy(d)
        new_tier_card['primuses'] = [fix_shards_format(x) for x in new_tier_card['primuses']]
        new_tier_card['properties']['min_replicas'] = new_tier_card['properties'].get('min_replicas', 0)
        new_tier = Tier(self.db, new_tier_card)

        self.tiers[new_tier.name] = new_tier

    def remove_tier(self, tier):
        assert (self.has_tier(tier))
        del self.tiers[tier]

    def rename_tier(self, old_name, new_name):
        assert (self.has_tier(old_name))
        assert (not self.has_tier(new_name))
        tier = self.tiers[old_name]
        del self.tiers[old_name]
        tier.name = new_name
        self.tiers[tier.name] = tier

    def resize_tier(self, name, new_size, primuses_data=None):
        """
            Resize tier to new size. Currently supported to cases:
                - we have exactly one primus and this function changes number of shards on this primus
                - param <primuses_data> is not None and contains list of pairs <primus, [shards]>
        """

        assert (new_size > 0)
        assert (self.has_tier(name))
        tier = self.tiers[name]

        if primuses_data is None:
            assert (len(self.tiers[name].primuses) == 1)
            tier.primuses[0]['shards'] = range(new_size)
        else:
            tier.primuses = map(lambda (name, shards): CardNode.create_from_dict(dict(name=name, shards=shards)),
                                primuses_data)

        tier.init_shard_ids()

    def get_tiers_groups_and_intlookups(self):
        tiers = {tier: (set(), set()) for tier in self.tiers.keys()}
        for intlookup in self.db.intlookups.get_intlookups():
            if intlookup.tiers is None:
                break
            if intlookup.base_type is not None:
                intlookup_groups = {intlookup.base_type}
            else:
                intlookup_groups = set()
                for basesearch in intlookup.get_base_instances_iterator():
                    intlookup_groups.add(basesearch.type)
            # TODO: it seems we don't need to think in terms of shards here
            for tier in intlookup.tiers:
                tiers[tier][0].update(intlookup_groups)
                tiers[tier][1].add(intlookup)
        # normalize
        tiers = {self.tiers[tier]:
                     ([self.db.groups.get_group(x) for x in groups],
                      list(intlookups))
                 for tier, (groups, intlookups) in tiers.items()
                 }
        return tiers

    def get_tier_groups_and_intlookups(self, tier):
        intlookups = self.db.intlookups.get_intlookups()
        result_intlookups = set()
        result_groups = set()
        for intlookup in intlookups:
            if intlookup.tiers is None or tier.name not in intlookup.tiers:
                continue
            result_intlookups.add(intlookup)
            if intlookup.base_type is not None:
                result_groups.add(intlookup.base_type)
            else:
                for basesearch in intlookup.get_base_instances_iterator():
                    result_groups.add(basesearch.type)

        result_intlookups = list(result_intlookups)
        result_groups = [self.db.groups.get_group(x) for x in result_groups]
        return result_groups, result_intlookups

    def primus_int_count(self, s):
        if isinstance(s, int):
            return s, None

        try:
            ast = compiler.parse(s)
        except Exception, e:
            raise Exception("Failed to parse line <%s> as python expression. Error: <%s>" % (s, str(e)))

        ast_expr = ast.getChildren()[1].getChildren()[0].getChildren()[0]

        def recurse_calc_ast(node):
            if isinstance(node, (compiler.ast.Add, compiler.ast.Sub)):
                left_result, left_tiers = recurse_calc_ast(node.left)
                right_result, right_tiers = recurse_calc_ast(node.right)

                if isinstance(node, compiler.ast.Add):
                    result = left_result + right_result
                    tiers = left_tiers + right_tiers
                elif isinstance(node, compiler.ast.Sub):
                    result = left_result - right_result
                    tiers = left_tiers + right_tiers
                else:
                    raise Exception("OOPS")

                return result, tiers
            elif isinstance(node, (compiler.ast.Mul, compiler.ast.Div)):
                left_result, left_tiers = recurse_calc_ast(node.left)
                right_result, right_tiers = recurse_calc_ast(node.right)

                assert left_tiers == [] or right_tiers == []
                assert isinstance(node, compiler.ast.Mul) or right_tiers == []

                if isinstance(node, compiler.ast.Mul):
                    result = left_result * right_result
                    tiers = left_tiers + right_tiers
                elif isinstance(node, compiler.ast.Div):
                    assert left_result % right_result == 0
                    result = left_result / right_result
                    tiers = left_tiers + right_tiers
                else:
                    raise Exception("OOPS")

                return result, tiers
            elif isinstance(node, compiler.ast.Name):
                # add groupnames, indicating number of instances
                if self.has_tier(node.name):
                    result = self.get_total_shards(node.name)
                    tiers = [node.name]
                elif self.db.intlookups.has_intlookup(node.name):
                    result = len(self.db.intlookups.get_intlookup(node.name).get_base_instances())
                    tiers = []
                elif self.db.groups.has_group(node.name):
                    result = len(self.db.groups.get_group(node.name).get_instances())
                    tiers = []
                else:
                    raise Exception("Do not know how to convert <%s> to number of instances" % node.name)

                return result, tiers
            elif isinstance(node, compiler.ast.Const):
                result = node.value
                tiers = []

                return result, tiers
            else:
                raise Exception("Unknown class %s" % node.__class__)

        result, tiers = recurse_calc_ast(ast_expr)
        uniq_tiers = []
        for tier in tiers:
            if tier not in uniq_tiers:
                uniq_tiers.append(tier)

        if len(uniq_tiers) == 0:
            return result, None
        else:
            return result, uniq_tiers

    def update(self, smart=False):
        # smart updating not enabled: update all even with smart
        if self.db.version >= "2.2.51":
            result = [x.as_dict() for x in sorted(self.tiers.values(), key=lambda x: x.name)]
            with open(self.TIERS_FILE, 'w') as f:
                f.write(json.dumps(result, indent=4, sort_keys=True))
        else:
            save_card_node(sorted(self.tiers.values(), cmp=lambda x, y: cmp(x.name, y.name)), self.TIERS_FILE,
                           self.SCHEME_FILE)

    def fast_check(self, timeout):
        # if we were created => we are ok
        # but, as noone check logic, let's try to get real objects
        self.get_tiers()
