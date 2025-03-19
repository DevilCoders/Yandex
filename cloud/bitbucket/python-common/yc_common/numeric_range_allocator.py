import random
from typing import Tuple, Optional, List

from yc_common.clients.kikimr import Variable

from yc_common.clients.kikimr.client import KikimrTable, KikimrCursorSpec
from yc_common.clients.kikimr.sql import SqlWhere, SqlIn, SqlInsertValues, SqlCondition, SqlCompoundKeyMatch, \
    SqlCursorCondition, SqlCursorOrderLimit
from yc_common.exceptions import Error, LogicalError
from yc_common import logging
from yc_common.models import Model, IntType, StringType


DEFAULT_MAX_RANGE_LENGTH = 10000
DEFAULT_ALLOCATE_RETRY_TIMES = 2
DEFAULT_PICK_SIZE = 5

log = logging.get_logger(__name__)


class LiteSparseRange:

    def __init__(self, start, stop):
        super().__init__()
        self.__data = [range(start, stop)]

    def remove(self, elem):
        idx = -1
        for i, rng in enumerate(self.__data):
            if elem in rng:
                idx = i
                break

        if idx < 0:
            return

        rng = self.__data[idx]
        if rng[0] == elem and rng[-1] == elem:
            del self.__data[idx]

        elif elem == rng[0]:
            self.__data[idx] = range(rng[0]+1, rng[-1]+1)

        elif elem == rng[-1]:
            self.__data[idx] = range(rng[0], rng[-1])

        else:
            self.__data[idx] = range(elem+1, rng[-1]+1)
            self.__data.insert(idx, range(rng[0], elem))

    def is_empty(self):
        return not self.__data

    def get_n(self, max_count):
        result = []
        data_iter = iter(self.__data)
        while len(result) < max_count:
            try:
                rng = next(data_iter)
            except StopIteration:
                break
            need = max_count - len(result)
            result += rng[:need]
        return result


class NumericRangeDescriptor(Model):
    prefix = IntType()
    start = IntType(required=True)
    end = IntType(required=True)

    def order_key(self):
        return self.prefix, self.start

    def as_tuple(self):
        return self.prefix, self.start, self.end

    def __hash__(self):
        return hash(self.as_tuple())

    def __eq__(self, other):
        return self.as_tuple() == other.as_tuple()


class NumericRange(NumericRangeDescriptor):
    """
        This model is schema for kikimr table that stores numeric ranges. In one table
        that uses this schema you can store ranges for resources of the same type. One
        table for subnet adresses, one table for route targets, etc...

        Primary key consists of 'resource_id', 'prefix' and 'start' columns.
        If prefix has 1 to 1 connection primary key will degrade to 'resource_id' and
        'element' columns.

        Columns meaning:
            resource_id - Id of resource that owns this range of elements(subnet that owns ip-addresses).
            prefix      - secondary field that can be used like range ofset(first 64bits of IPV6 subnet),
                        or can contain some special information that describe resource or range.
            start       - first element of range.
            end         - last element of range.
            filled      - range attribute that says if it has empty elements in.
    """
    class Occupancy:
        NOT_FILLED = 0
        FILLED = 1
        NEARLY_FILLED = 2  # Not used yet
        ALL = [NOT_FILLED, FILLED, NEARLY_FILLED]

    resource_id = StringType(required=True)
    filled = IntType(required=True, choices=Occupancy.ALL)


def get_overlapping_ranges(ranges: List[NumericRangeDescriptor]) -> List[NumericRangeDescriptor]:
    """ Check if there is a range which overlaps with other. Return one of overlapping if exist or None. """
    result = []
    sorted_ranges = sorted(ranges, key=NumericRangeDescriptor.order_key)
    prev_added = False
    for i in range(1, len(ranges)):
        c_range = sorted_ranges[i]
        prev_range = sorted_ranges[i - 1]
        # Note(@svartapetov): Now we use logic that ranges under different prefixes are not overlapping
        # This may one day change.
        if c_range.prefix == prev_range.prefix and c_range.start <= prev_range.end:
            if not prev_added:
                result.append(prev_range)
            result.append(c_range)
            prev_added = True
        else:
            prev_added = False

    return result


class AllocatedElement(Model):
    """
        This model is schema for kikimr table that stores allocated elements
        from ranges that stores in NumericRange table.

        Primary key consists of 'resource_id', 'prefix' and 'element' columns.
        If prefix has 1 to 1 connection primary key will degrade to 'resource_id' and 'element' columns.

        Columns meaning:
            resource_id - Id of the resource that owns this range of elements (subnet that owns ip-addresses).
            owner_id    - Id of the resource that owns this particular element (network attachment that owns this ip-address)
            prefix      - secondary field that can be used like range offset (first 64bits of IPV6 subnet),
                        or can contain some special information that describe resource or range.
            element     - value of allocated element.
            timestamp   - time after which the element is considered unallocated(NOT IMPLEMENTED YET)
    """

    resource_id = StringType(required=True)
    prefix = IntType()
    element = IntType(required=True)
    owner_id = StringType(required=True)
    timestamp = IntType()


class ElementOutOfRangeError(LogicalError):
    def __init__(self, element):
        super().__init__(
            "Element {!r} does not belong to any available range.", element
        )


class NoAvailableElementsFoundError(Error):
    def __init__(self):
        super().__init__(
            "No elements found in any available range."
        )


class ElementAlreadyInUseError(Error):
    def __init__(self, element):
        super().__init__(
            "Element {!r} already allocated.", element
        )


class RangeOverlapsError(Error):
    def __init__(self, start, end, resource_id):
        super().__init__(
            "Range with scope {!r}..{!r} overlaps with other ranges of {!r}.", start, end, resource_id
        )


# TODO (CLOUD-14764): Use only one error
class RangesOverlapError(RangeOverlapsError):
    def __init__(self, ranges_info, resource_id):
        super().__init__(
            "Range with scope {!r} overlaps with other ranges of {!r}.", ranges_info, resource_id
        )


class InvalidRangeError(Error):
    def __init__(self, start, end, resource_id):
        super().__init__(
            "Range with scope {!r}..{!r} is unavailable for {!r}.", start, end, resource_id
        )


# TODO (CLOUD-14764): Use only one error
class InvalidRangesError(InvalidRangeError):
    def __init__(self, ranges_info, resource_id):
        super().__init__(
            "Range with scope {!r} is unavailable for {!r}.", ranges_info, resource_id
        )


class RangeIsNotEmptyError(Error):
    def __init__(self, start, end, resource_id):
        super().__init__(
            "Range with scope {!r}..{!r} for {!r} is not empty.", start, end, resource_id
        )


# TODO (CLOUD-14764): Use only one error
class RangesAreNotEmptyError(RangeIsNotEmptyError):
    def __init__(self, ranges_info, resource_id):
        super().__init__(
            "One ore more ranges with scopes {!r} for {!r} are not empty.", ranges_info, resource_id
        )


class AllocatorIsNotEmptyError(Error):
    def __init__(self, resource_id, prefix=None):
        super().__init__(
            "One of ranges for {!r} (prefix {!r}) has elements allocated", resource_id, prefix
        )


class RangeAlreadyExistsError(Error):
    def __init__(self, start, end, resource_id):
        super().__init__(
            "Range with scope {!r}..{!r} for {!r} already exists.", start, end, resource_id
        )


# TODO: Use only one error
class RangesAlreadyExistError(RangeAlreadyExistsError):
    def __init__(self, ranges_info, resource_id):
        super().__init__(
            "Range with scope {!r} for {!r} already exists.", ranges_info, resource_id
        )


class TooManyConflictsError(LogicalError):
    def __init__(self, start, end, resource_id):
        super().__init__(
            "Too many conflicts during allocation from range with scope {!r}..{!r} for {!r}. "
            "Consider increasing pick_size.", start, end, resource_id
        )


class NumericRangeAllocator:

    def __init__(
        self, range_table: KikimrTable, alloc_table: KikimrTable,
        resource_id, pick_size=DEFAULT_PICK_SIZE, max_range_length=DEFAULT_MAX_RANGE_LENGTH,
        allocate_retry_times=DEFAULT_ALLOCATE_RETRY_TIMES, element_formatter=lambda pfx, elem: elem
    ):
        self.range_table = range_table
        self.alloc_table = alloc_table
        self.resource_id = resource_id
        self.pick_size = pick_size
        self.allocate_retry_times = allocate_retry_times
        self.max_range_length = max_range_length
        self.element_formatter = element_formatter
        if self.max_range_length > DEFAULT_MAX_RANGE_LENGTH:
            log.warning(
                "Allocator won't work if you allocate more than %s elements.",
                DEFAULT_MAX_RANGE_LENGTH
            )
        self.filled_ranges = []

    def __check_allocation_exists(self, tx, prefix, candidate_element):
        with tx.table_scope(alloc=self.alloc_table):
            alloc_args = self.__where_with_prefix(prefix)
            alloc_args.and_condition("element = ?", candidate_element)

            return tx.select_one("SELECT * FROM $alloc_table ?", alloc_args, model=AllocatedElement)

    def __try_use_element(self, tx, owner_id, prefix, candidate_element) -> Tuple[bool, Optional[AllocatedElement]]:
        # Checking for possible race: concurrent allocations could already
        # picked the same element and commited (!) usage of it (CLOUD-9463).
        existing_element = self.__check_allocation_exists(tx, prefix, candidate_element)
        if existing_element is not None:
            return False, existing_element

        with tx.table_scope(alloc=self.alloc_table):
            allocated = {
                "resource_id": self.resource_id,
                "prefix": prefix,
                "element": candidate_element,
                "owner_id": owner_id,
            }
            tx.insert_object("INSERT INTO $alloc_table", allocated, table=self.alloc_table)
        return True, None

    def allocate_specific(self, tx, owner_id, element, prefix=None):
        """Allocates specific element from range."""

        searched_range = self.__get_specific_range(element, prefix)
        prefix = searched_range.prefix

        _, existing_element = self.__try_use_element(tx, owner_id, prefix, element)
        if existing_element is not None:
            if existing_element.owner_id != owner_id:
                raise ElementAlreadyInUseError(self.element_formatter(prefix or 0, element))
            else:
                return existing_element.prefix, existing_element.element

        return prefix, element

    def try_allocate_specific(self, tx, owner_id, element, prefix=None):
        searched_range = self.__get_specific_range(element, prefix)
        prefix = searched_range.prefix

        existing_element = self.__check_allocation_exists(tx, prefix, element)
        if existing_element is not None:
            if existing_element.owner_id != owner_id:
                raise ElementAlreadyInUseError(self.element_formatter(prefix or 0, element))

    def allocate_random(self, tx, owner_id, prefix=None):
        retry_times = 0
        while True:
            try:
                return self.__allocate_random(tx, owner_id, prefix=prefix)
            except TooManyConflictsError as e:
                # candidate_elements list might be too short, and parallel requests may use it up already,
                # repeat same request with possibly different range.
                if retry_times < self.allocate_retry_times:
                    retry_times += 1
                    continue
                else:
                    raise e

    def __allocate_random(self, tx, owner_id, prefix=None):
        """Allocates some element from range."""

        # This loop must iter once, but we need it
        # to avoid race, when no allocator marks
        # filled range as filled, and we need fix
        # it on fly.
        # If all ranges are filled we will break
        # this loop by error, raised by
        # __get_underfilled_range function

        while True:
            searched_range = self.__get_underfilled_range(prefix)

            unused_elements = self.__get_range_unused_elements(searched_range, searched_range.prefix)
            if not unused_elements.is_empty():
                break

            log.warning(
                "Range for %s with start %s and end %s was not marked as filled, will do it in this transaction.",
                self.resource_id, searched_range.start, searched_range.end
            )
            self.filled_ranges.append(searched_range)

        prefix = searched_range.prefix

        # Here we get first %pick_size% empty elements and then shuffle them.
        # That's how we try to minimize possibility of collision with other
        # transactions (TransactionLocksInvalidatedError).
        candidate_elements = unused_elements.get_n(self.pick_size)
        random.shuffle(candidate_elements)

        element = None
        used_elements_by_other_transactions = 0
        for candidate in candidate_elements:
            inserted_new, _ = self.__try_use_element(tx, owner_id, prefix, candidate)
            if inserted_new:
                element = candidate
                break
            else:
                used_elements_by_other_transactions += 1

        if element is None:
            raise TooManyConflictsError(searched_range.start, searched_range.end, searched_range.resource_id)

        # Mark range filled if we take last element
        if len(candidate_elements) - used_elements_by_other_transactions == 1:
            self.filled_ranges.append(searched_range)

        if self.filled_ranges:
            range_args = self.__where_with_prefix(prefix)
            in_query = SqlIn("start", [Variable(numeric_range["start"], self.range_table.column_type("start")) for numeric_range in self.filled_ranges])
            range_args.and_condition(in_query)

            with tx.table_scope(range=self.range_table):
                tx.query("UPDATE $range_table SET filled = ? ?",
                         Variable(NumericRange.Occupancy.FILLED, self.range_table.column_type("filled")),
                         range_args)

            self.filled_ranges = []

        return (prefix, element)

    def release(self, tx, owner_id, element, prefix):
        """Returns element to range. """

        searched_range = self.__get_specific_range(element, prefix)

        with tx.table_scope(numeric_range=self.range_table, alloc=self.alloc_table):
            alloc_args = self.__where_with_prefix(prefix)
            alloc_args.and_condition("element = ?", element)
            alloc_args.and_condition("owner_id = ?", owner_id)
            tx.query("DELETE FROM $alloc_table ?", alloc_args)

            if searched_range.filled == NumericRange.Occupancy.FILLED:
                range_args = self.__where_with_prefix(prefix)
                range_args.and_condition(
                    "start = ? AND end = ?",
                    searched_range.start, searched_range.end
                )
                tx.query("UPDATE $numeric_range_table SET filled = ? ?",
                         Variable(NumericRange.Occupancy.NOT_FILLED, self.range_table.column_type("filled")), range_args)

    def allocations(self, tx=None):
        if tx is None:
            tx = self.alloc_table.client

        result = []
        with tx.table_scope(self.alloc_table):
            for element in tx.select_paged(
                    KikimrCursorSpec(self.alloc_table),
                    "SELECT * FROM $table WHERE resource_id = ? AND ? ?",
                    self.resource_id,
                    SqlCursorCondition(), SqlCursorOrderLimit(),
                    model=AllocatedElement):
                formatted_element = self.element_formatter(element.prefix, element.element)
                result.append((formatted_element, element.owner_id))

        return result

    # DEPRECATED (CLOUD-14764): singular method to be removed. Use plural instead
    def append_range(self, tx, start, end, prefix):
        """Add one more range of elements."""

        self._validate_range(start, end, prefix)
        with tx.table_scope(numeric_range=self.range_table):
            where_args = self.__where_with_prefix(prefix)
            ranges = tx.select("SELECT * FROM $numeric_range_table ? ORDER BY start", where_args, model=NumericRange)
            for ival in ranges:
                if ival["end"] == end and ival["start"] == start:
                    raise RangeAlreadyExistsError(self.element_formatter(prefix or 0, start), self.element_formatter(prefix or 0, end), self.resource_id)

                if ival["end"] < start:
                    continue

                if ival["start"] > end:
                    break
                else:
                    raise RangeOverlapsError(self.element_formatter(prefix or 0, start), self.element_formatter(prefix or 0, end), self.resource_id)

            tx.insert_object("REPLACE INTO $numeric_range_table", {
                "resource_id": self.resource_id,
                "prefix": prefix,
                "start": start,
                "end": end,
                "filled": NumericRange.Occupancy.NOT_FILLED,
            })

    def get_all_ranges(self, tx) -> List[NumericRange]:
        with tx.table_scope(numeric_range=self.range_table):
            where_args = self.__where_with_prefix(None)
            all_ranges = tx.select("SELECT * FROM $numeric_range_table ? ORDER BY start", where_args, model=NumericRange)
        return list(all_ranges)

    def append_ranges(self, tx, ranges: List[NumericRangeDescriptor]):
        """Add ranges of elements.
        @param tx - transaction
        @param ranges - list of tuples (start, end, prefix) describing each range to add.
        """

        self._validate_ranges(ranges)

        to_add = [NumericRange.new(resource_id=self.resource_id, filled=NumericRange.Occupancy.NOT_FILLED,
                                   prefix=_range.prefix, start=_range.start, end=_range.end)
                  for _range in ranges]

        with tx.table_scope(numeric_range=self.range_table):
            where_args = self.__where_with_prefix(None)
            all_ranges = tx.select("SELECT * FROM $numeric_range_table ? ORDER BY start", where_args, model=NumericRange)
            overlap_ranges = set(to_add) & set(all_ranges)
            if overlap_ranges:
                raise RangesAlreadyExistError(self._format_range_list_info(list(overlap_ranges)), self.resource_id)
            overlapping_ranges = get_overlapping_ranges(all_ranges + to_add)
            if overlapping_ranges:
                raise RangesOverlapError(self._format_range_list_info(overlapping_ranges), self.resource_id)
            insert_values = SqlInsertValues(NumericRange.columns(), [x.to_kikimr_values() for x in to_add])
            tx.query("REPLACE INTO $numeric_range_table ?", insert_values)

    # DEPRECATED (CLOUD-14764): singular method to be removed. Use plural instead
    def remove_range(self, tx, start, end, prefix=None):
        """Remove range of elements."""

        with tx.table_scope(numeric_range=self.range_table, alloc=self.alloc_table):
            range_args = self.__where_with_prefix(prefix)
            range_args.and_condition("start = ?", start)
            deleting_range = tx.select_one(
                "SELECT * FROM $numeric_range_table ?", range_args, model=NumericRange
            )
            if deleting_range is None or deleting_range.end != end:
                raise InvalidRangeError(self.element_formatter(prefix or 0, start), self.element_formatter(prefix or 0, end), self.resource_id)

            alloc_args = self.__where_with_prefix(prefix)
            alloc_args.and_condition("element >= ? AND element <= ?", start, end)
            allocated = tx.select_one("SELECT COUNT(element) AS count FROM $alloc_table ?", alloc_args)
            if allocated["count"] > 0:
                raise RangeIsNotEmptyError(self.element_formatter(prefix or 0, start), self.element_formatter(prefix or 0, end), self.resource_id)

            tx.query("DELETE FROM $numeric_range_table ?", range_args)

    def remove_ranges(self, tx, ranges: List[NumericRangeDescriptor]):
        """Remove ranges of elements.
        @param tx - transaction
        @param ranges - list of tuples (start, end, prefix) describing each range to remove.
        """
        self._validate_ranges(ranges)
        where_in_range_cond = self._get_where_condition_from_ranges(ranges)
        range_args = SqlWhere()
        range_args.and_condition("resource_id = ?", self.resource_id)
        range_args.and_condition(where_in_range_cond)
        with tx.table_scope(numeric_range=self.range_table, alloc=self.alloc_table):
            deleting_ranges = tx.select(
                "SELECT * FROM $numeric_range_table ? ", range_args, model=NumericRange
            )
            kiki_sorted = sorted(deleting_ranges, key=NumericRangeDescriptor.as_tuple)
            req_sorted = sorted(ranges, key=NumericRangeDescriptor.as_tuple)
            # Note that there should never be len(deleting_ranges) > len(ranges) since all primary keys are in request
            if len(deleting_ranges) != len(ranges) \
               or kiki_sorted != req_sorted:
                bad_requests = set(req_sorted) - set(kiki_sorted)
                if not bad_requests:
                    raise LogicalError("Deleting request `{}` for {} generate more ranges than was expected.".format(self._format_range_list_info(ranges), self.resource_id))
                raise InvalidRangesError(self._format_range_list_info(bad_requests), self.resource_id)
            # Check if there was some allocations within ranges.
            alloc_args = SqlWhere()
            alloc_args.and_condition("resource_id = ?", self.resource_id)
            alloc_args.and_condition(self._get_where_condition_from_ranges_elems(ranges))
            allocated = tx.select_one("SELECT COUNT(element) AS count FROM $alloc_table ?", alloc_args)
            if allocated["count"] > 0:
                raise RangesAreNotEmptyError(self._format_range_list_info(ranges), self.resource_id)
            tx.query("DELETE FROM $numeric_range_table ?", range_args)

    def destroy_all_ranges(self, tx, prefix=None):
        """Destroy all ranges associated with this resource id if they are empty."""

        with tx.table_scope(numeric_range=self.range_table, alloc=self.alloc_table):
            alloc_args = self.__where_with_prefix(prefix)
            alloc_args.and_condition("resource_id = ?", self.resource_id)

            allocated = tx.select_one("SELECT COUNT(element) AS count FROM $alloc_table ?", alloc_args)
            if allocated["count"] > 0:
                raise AllocatorIsNotEmptyError(self.resource_id, prefix)

            range_args = self.__where_with_prefix(prefix)
            tx.query("DELETE FROM $numeric_range_table ?", range_args)

    def __get_specific_range(self, element, prefix) -> NumericRange:
        """Validates if range with this element presents and returns it."""

        where_args = self.__where_with_prefix(prefix)
        where_args.and_condition("start <= ?", element)
        specific_range = self.range_table.client.select_one(
            "SELECT * FROM $table ? ORDER BY start DESC LIMIT 1",
            where_args, model=NumericRange
        )
        if not specific_range or element > specific_range.end:
            raise ElementOutOfRangeError(self.element_formatter(prefix or 0, element))

        return specific_range

    def __get_underfilled_range(self, prefix) -> NumericRange:
        """Gets first range for resource that is not already filled."""

        where_args = self.__where_with_prefix(prefix)
        where_args.and_condition("filled = ?", NumericRange.Occupancy.NOT_FILLED)
        for numeric_range in self.filled_ranges:
            where_args.and_condition("start != ?", numeric_range.start)

        underfilled_range = self.range_table.client.select_one(
            "SELECT * FROM $table ? LIMIT 1",
            where_args, model=NumericRange, commit=True
        )
        if underfilled_range is None:
            raise NoAvailableElementsFoundError()

        return underfilled_range

    def __get_range_unused_elements(self, numeric_range, prefix):
        """Gets all non allocated elements from range."""

        where_args = self.__where_with_prefix(prefix)
        where_args.and_condition(
            "element >= ? AND element <= ?",
            numeric_range.start, numeric_range.end
        )
        unused_elements = LiteSparseRange(numeric_range.start, numeric_range.end+1)
        for i in self.alloc_table.client.select("SELECT element FROM $table ?", where_args):
            unused_elements.remove(i["element"])

        return unused_elements

    def __where_with_prefix(self, prefix) -> SqlWhere:
        """Makes where statement with or without prefix field."""

        where_args = SqlWhere()
        where_args.and_condition("resource_id = ?", self.resource_id)
        if prefix is not None:
            where_args.and_condition("prefix = ?", prefix)

        return where_args

    def _get_primary_keys_range(self):
        return self.range_table.current_spec().primary_keys

    def _is_prefix_primary_range(self):
        return 'prefix' in self._get_primary_keys_range()

    def _get_primary_keys_alloc(self):
        return self.alloc_table.current_spec().primary_keys

    def _is_prefix_primary_alloc(self):
        return 'prefix' in self._get_primary_keys_alloc()

    def _get_range_to_element_condition_contain(self, _range: NumericRangeDescriptor) -> Tuple[str, Tuple]:
        template = "element BETWEEN ? AND ?"
        res_args = _range.start, _range.end

        if self._is_prefix_primary_alloc():
            template += " AND prefix = ?"
            res_args += (_range.prefix,)
        return (template, res_args)

    def _get_where_condition_from_ranges_elems(self, ranges: List[NumericRangeDescriptor]):
        res_template = "FALSE"
        res_args = ()
        for _range in ranges:
            c_template, c_args = self._get_range_to_element_condition_contain(_range)
            res_template += " OR " + c_template
            res_args += c_args
        return SqlCondition(res_template, *res_args)

    def _get_where_condition_from_ranges(self, ranges: List[NumericRangeDescriptor]) -> SqlCompoundKeyMatch:
        to_compare_fields = sorted(set(self._get_primary_keys_range()) & set(NumericRangeDescriptor.columns()))
        # TODO: Use SqlInMany after KIKIMR-5598.
        return SqlCompoundKeyMatch(to_compare_fields, [[v for k, v in x.items() if k in to_compare_fields] for x in ranges])

    def _validate_range(self, start, end, prefix):
        if start > end or end - start > self.max_range_length:
            raise InvalidRangeError(self.element_formatter(prefix or 0, start), self.element_formatter(prefix or 0, end), self.resource_id)

    def _validate_ranges(self, ranges: List[NumericRangeDescriptor]):
        for _range in ranges:
            self._validate_range(_range.start, _range.end, _range.prefix)

    def _format_range_list_info(self, ranges: List[NumericRangeDescriptor]):
        return "[{}]".format("; ".join("{start}..{end}".format(start=self.element_formatter(_range.prefix, _range.start),
                                                               end=self.element_formatter(_range.prefix, _range.end))
                             for _range in ranges))
