import datetime

from antirobot.cbb.cbb_django.cbb.library import db
from antirobot.cbb.cbb_django.cbb.library.common import (get_ip_from_string, get_range_from_net, ranges_types)
from antirobot.cbb.cbb_django.cbb.models import (BLOCK_VERSIONS, HISTORY_VERSIONS, Group)
from antirobot.cbb.cbb_django.cbb.library.errors import (
    BadGroupTypeError, EmptyRangeError,
    ExceptionEqualsNetError, ExceptionMismatchError,
    ExceptionToInternalError, InvalidNetRangeError,
    InvalidRangeError, OverlapExceptionsError,
    OverlapRangeError, RangeAlreadyExistsError,
    WrongOperationError
)
from django.utils.translation import ugettext_lazy as _
from ipaddr import IPAddress, summarize_address_range
from sqlalchemy import or_
from sqlalchemy.orm.exc import MultipleResultsFound, NoResultFound


def cbb_error(scold, error):
    if scold:
        raise error


def optimize_net(ranges_obj):
    ranges_list = []

    for rng in ranges_obj:
        ranges_list.append((int(rng.get_rng_start()), int(rng.get_rng_end())))

    ranges_list.sort()

    p_start = None
    p_end = None
    ranges_result = []

    for (start, end) in ranges_list:
        if p_start is None:
            p_start = start
        if p_end is None:
            p_end = end
        if start <= p_end + 1 and end > p_end:
            p_end = end
        if start > p_end + 1:
            ranges_result.append((p_start, p_end))
            p_start = start
            p_end = end

    if p_start is not None and p_end is not None:
        ranges_result.append((p_start, p_end))

    return ranges_result


# Ip ranges actions
def get_net(net_ip, net_mask, group_id, version=4):
    (start, end) = get_range_from_net(net_ip, net_mask, int(version))
    RangeClass = BLOCK_VERSIONS[version]

    try:
        return RangeClass.query.filter_by(is_exception=False).filter_by(rng_start=start).filter_by(rng_end=end).filter_by(group_id=group_id).one()
    except (NoResultFound, MultipleResultsFound):
        return None


def get_exceptions_for_net(net, mask, version=4, groups=()):
    # TODO: version and groups switch
    (start, end) = get_range_from_net(net, mask, version)
    RangeClass = BLOCK_VERSIONS[version]

    return RangeClass.query.filter_by(is_exception=True).filter(RangeClass.rng_start <= end).filter(RangeClass.rng_end >= start).filter(RangeClass.group_id.in_(groups)).all()


def get_single_range(rng_start, rng_end, group_id, version):
    RangeClass = BLOCK_VERSIONS[version]
    rng_start = get_ip_from_string(rng_start, version)
    rng_end = get_ip_from_string(rng_end, version)

    return RangeClass.query.filter_by(rng_start=rng_start, rng_end=rng_end, group_id=group_id).one()


def get_ranges(rng_start=None, rng_end=None, version=4, groups=(), is_exception=None, block_type=None, get_all=True, is_internal=None, exclude_expired=False):
    """
    Возвращает все интервалы, которые пересекаются с данным
    На вход - строки с ip
    """
    RangeClass = BLOCK_VERSIONS[version]

    ranges_result = db.main.session.query(RangeClass)

    if exclude_expired:
        ranges_result = ranges_result.filter(or_(RangeClass.expire.is_(None), RangeClass.expire >= datetime.datetime.now()))

    if is_exception is not None:
        ranges_result = ranges_result.filter_by(is_exception=is_exception)

    if rng_start:
        rng_start = get_ip_from_string(rng_start, version)

        if rng_end:
            rng_end = get_ip_from_string(rng_end, version)

            if rng_end < rng_start:
                raise InvalidRangeError(_("Range's start is greater than end"))
            ranges_result = ranges_result.filter(RangeClass.rng_start <= rng_end).filter(RangeClass.rng_end >= rng_start)
        else:
            ranges_result = ranges_result.filter_by(rng_start=rng_start)

    if groups:
        ranges_result = ranges_result.filter(RangeClass.group_id.in_(groups))

    if block_type in ["cidr", "range", "single_ip"]:
        ranges_result = ranges_result.filter_by(block_type=block_type)

    if get_all:
        return ranges_result.all()
    else:
        return ranges_result


def get_external_ranges(rng_start, rng_end, version):
    RangeClass = BLOCK_VERSIONS[version]

    return RangeClass.query.join(Group, Group.id == RangeClass.group_id)\
            .filter(RangeClass.rng_start <= rng_end)\
            .filter(RangeClass.rng_end >= rng_start)\
            .filter(Group.is_active is True)\
            .filter(Group.is_internal is False)\
            .all()


def del_ip_range(rng_start, rng_end, group, user="", operation_descr="", version=4, net=True, scold=True):
    group_type = ranges_types[group.default_type]

    if group_type in ["cidr", "single_ip"]:
        return del_ip_range_exact(rng_start, rng_end, group, user, operation_descr, version, net, scold)
    elif group_type == "range":
        return del_ip_range_inexact(rng_start, rng_end, group, user, operation_descr, version)
    else:
        raise BadGroupTypeError(_("Wrong group type"))


def del_ip_range_exact(rng_start, rng_end, group, user="", operation_descr="", version=4, net=True, scold=True):
    """
    net - для групп с сетями: передаем сеть в виде адрес маска или в ввиде
    промежутка
    Строчки на вход.
    Точное удаление - находим !один! интервал, точно совпадающий с данным,
    и просто его удаляем, не забыв про историю и группу
    """
    RangeClass = BLOCK_VERSIONS[version]

    group_type = ranges_types[group.default_type]
    if group_type == "cidr" and net:
        (rng_start, rng_end) = get_range_from_net(rng_start, rng_end)
    else:
        rng_start = get_ip_from_string(rng_start, version)
        rng_end = get_ip_from_string(rng_end, version)

    if group_type == "cidr":
        exceptions = RangeClass.query.filter(
            RangeClass.rng_start >= rng_start). \
            filter(RangeClass.rng_end <= rng_end). \
            filter_by(group_id=group.id, is_exception=True).all()
        for ex in exceptions:
            db.main.session.delete(ex)
    try:
        block_to_delete = RangeClass.query.filter_by(rng_start=rng_start, rng_end=rng_end, group_id=group.id, is_exception=False).one()
    except NoResultFound:
        cbb_error(scold, InvalidRangeError(_("No such range found")))
        return

    db.main.session.delete(block_to_delete)

    HISTORY_VERSIONS[version].add(block=block_to_delete, group=group, unblocked_by=user, unblock_description=operation_descr)


def del_ip_range_inexact(rng_start, rng_end, group, user="", operation_descr="", version=4):
    """
    Удаление с перестройкой - находим все интервалы этой группы, которые
    пересекаются с данным, и режем их так, чтобы они с данным пересекаться
    перестали. В историю добавляем одну запись - про удаление данного интервала.
    А может быть и не одну.
    """

    rng_start_int = get_ip_from_string(rng_start, version, "int")
    rng_end_int = get_ip_from_string(rng_end, version, "int")

    rng_start = get_ip_from_string(rng_start, version)
    rng_end = get_ip_from_string(rng_end, version)

    RangeClass = BLOCK_VERSIONS[version]
    ranges_crossed = get_ranges(rng_start, rng_end, version, [group.id], False)
    for rng in ranges_crossed:
        if int(rng.get_rng_start()) < rng_start_int and int(rng.get_rng_end()) > rng_end_int:
            rng_new_start = get_ip_from_string(rng_end_int + 1, version)
            rng_new = RangeClass(
                rng_start=rng_new_start,
                rng_end=rng.rng_end,
                group_id=rng.group_id,
                is_exception=rng.is_exception,
                created=rng.created,
                block_descr=rng.block_descr,
                expire=rng.expire,
                user=rng.user,
                block_type=rng.block_type
            )
            rng.set_rng_end(rng_start_int - 1)
            db.main.session.add(rng_new)
            continue
        elif int(rng.get_rng_start()) < rng_start_int:
            rng.set_rng_end(rng_start_int - 1)
        elif int(rng.get_rng_end()) > rng_end_int:
            rng.set_rng_start(rng_end_int + 1)
        else:
            db.main.session.delete(rng)
            HISTORY_VERSIONS[version].add(block=rng, group=group, unblocked_by=user, unblock_description=operation_descr)
    # TODO: добавлять в историю по записи для каждого из удаленных
    # промежутков из пересечения - сохранится юзер и для операции, и для
    # промежутка


def cut_internal_ranges(current_ranges, version):
    """
    Cuts internal ranges from current ranges and returns result.
    """
    # get internal groups
    internal_group_ids = Group.get_internal_ids()
    if not internal_group_ids:
        return current_ranges

    # get internal ranges
    internal_ranges = get_ranges(rng_start=current_ranges[0][0], rng_end=current_ranges[0][1], version=version, groups=internal_group_ids, is_exception=False)
    internal_ranges = optimize_net(internal_ranges)
    if not internal_ranges:
        return current_ranges
    # cut internal ranges from current ranges
    for (i_start, i_end) in internal_ranges:
        new_result = []
        for (c_start, c_end) in current_ranges:
            if i_start <= c_end and i_end >= c_start:
                if i_start > c_start:
                    new_result.append((c_start, i_start - 1))
                if i_end < c_end:
                    new_result.append((i_end + 1, c_end))
            else:
                new_result.append((c_start, c_end))
        if not new_result:
            return []
        current_ranges = new_result

    return current_ranges


def add_ip_range_common(rng_start, rng_end, group, block_descr="", user="", expire=None, version=4, scold_on_repeat=True):
    group_type = ranges_types[group.default_type]

    if group_type == "single_ip":
        return add_single_ip(rng_start, rng_end, group, block_descr, user, expire, version, scold_on_repeat)
    elif group_type == "range":
        return add_ip_range(rng_start, rng_end, group, block_descr, user, version, scold_on_repeat)
    elif group_type == "cidr":
        net = summarize_address_range(IPAddress(rng_start, version), IPAddress(rng_end, version))
        # TODO: повторяет текущую логику. можно просто резать на сети.
        if len(net) != 1:
            raise InvalidNetRangeError(_("Bad range for net given"))
        return add_net(str(net[0].ip), net[0].prefixlen, group, block_descr, user, version, scold_on_repeat)


def add_single_ip(rng_start, rng_end, group, block_descr="", user="", expire=None, version=4, scold_on_repeat=True):
    RangeClass = BLOCK_VERSIONS[version]
    expire = group.increase_expire(expire)
    rng_start = get_ip_from_string(rng_start, version)
    rng_end = get_ip_from_string(rng_end, version)
    rng_start_int = get_ip_from_string(rng_start, version, "int")
    rng_end_int = get_ip_from_string(rng_end, version, "int")
    ranges_result = [(rng_start_int, rng_end_int)]

    # check for duplicates and intersections
    if rng_start == rng_end:
        existing_ranges = RangeClass.q_block_crossings(rng_start, rng_end, group.id).limit(2).all()
    else:
        existing_ranges = get_ranges(rng_start, rng_end, version, [group.id], False, get_all=False).limit(2).all()

    if len(existing_ranges) > 1:
        # Удалим пересекающиеся диапазоны
        del_ip_range_inexact(rng_start, rng_end, group, user=user, version=version)
    elif len(existing_ranges) == 1:
        found_rng = existing_ranges[0]
        if found_rng.rng_start != rng_start or found_rng.rng_end != rng_end:
            # найден один, но с другими границами - пересечение
            # удалим пересекающиеся участки
            del_ip_range_inexact(rng_start, rng_end, group, user=user, version=version)
        elif (expire is None or found_rng.expire == expire):
            # найден с такими же границами, но без expire или с таким же expire
            cbb_error(scold_on_repeat, RangeAlreadyExistsError(_("This range already exists in this blocking")))
            return
        else:
            # найдем с такими же границами, и с другим expire. типа чтобы создать такую же, но с другим expire?
            db.main.session.delete(found_rng)  # грохаем старую блокировку
            db.main.session.flush()  # тут был баг

    # cut internal ranges
    if not group.is_internal:
        ranges_result = cut_internal_ranges(ranges_result, version)
        if not ranges_result:
            cbb_error(scold_on_repeat, EmptyRangeError(_("Impossible to block address because it is completely included to nonblocking addresses")))
            return
    else:
        external_ranges = get_external_ranges(rng_start, rng_end, version)
        if external_ranges:
            return external_ranges  # чтобы отдавать в интерфейс, типа из-за них не получилось добавить ToDo

    for (start, end) in ranges_result:
        r_start = get_ip_from_string(start, version)
        r_end = get_ip_from_string(end, version)
        new_block = RangeClass(rng_start=r_start, rng_end=r_end, group_id=group.id, user=user, block_descr=block_descr, expire=expire)
        db.main.session.add(new_block)
    group.update_updated()


def add_ip_range(rng_start, rng_end, group, block_descr="", user="", version=4, scold_on_repeat=True):
    # TODO: сохранять информацию по юзерам!
    RangeClass = BLOCK_VERSIONS[version]
    rng_start = get_ip_from_string(rng_start, version)
    rng_end = get_ip_from_string(rng_end, version)
    rng_start_int = get_ip_from_string(rng_start, version, "int")
    rng_end_int = get_ip_from_string(rng_end, version, "int")

    if ranges_types[group.default_type] != "range":
        raise BadGroupTypeError(_("Wrong group type"))

    # ToDO!!! это оптмизировать как в single через фильтры
    existing_range = RangeClass.query.filter(RangeClass.rng_start <= rng_start).filter(RangeClass.rng_end >= rng_end).filter_by(group_id=group.id).first()
    if existing_range:
        cbb_error(scold_on_repeat, OverlapRangeError(_("Found range that contains given")))
        return

    # ToDO!!! оптмизировать как в single через фильтры
    ranges_crossed = get_ranges(rng_start_int - 1, rng_end_int + 1, version, [group.id], False)
    # для перестройки добавляем в полученные промежутки тот, который собираемся
    # добавлять
    ranges_result = optimize_net(ranges_crossed + [RangeClass(rng_start=rng_start, rng_end=rng_end, group_id=group.id)])

    if not group.is_internal:
        ranges_result = cut_internal_ranges(ranges_result, version)
        if not ranges_result:
            cbb_error(scold_on_repeat, EmptyRangeError(_("Impossible to block address because it is completely included to nonblocking addresses")))
            return
    else:
        external_ranges = get_external_ranges(rng_start, rng_end, version)
        if external_ranges:
            return external_ranges

    if not ranges_result:
        cbb_error(scold_on_repeat, EmptyRangeError(_("Impossible to block address because it is completely included to nonblocking addresses")))
        return

    for (start, end) in ranges_result:
        r_start = get_ip_from_string(start, version)
        r_end = get_ip_from_string(end, version)
        new_block = RangeClass(rng_start=r_start, rng_end=r_end, group_id=group.id, user=user, block_descr=block_descr)
        db.main.session.add(new_block)

    for block in ranges_crossed:
        db.main.session.delete(block)
    group.update_updated()


def add_net(net_ip, net_mask, group, block_descr="", user="", version=4, scold_on_repeat=True):
    # int for 4, binary for 6
    RangeClass = BLOCK_VERSIONS[version]
    (rng_start, rng_end) = get_range_from_net(net_ip, net_mask, version)
    (rng_start_int, rng_end_int) = get_range_from_net(net_ip, net_mask, version, "int")
    net_result = [(rng_start_int, rng_end_int)]

    if ranges_types[group.default_type] != "cidr":
        raise BadGroupTypeError(_("Wrong group type"))

    existing_ranges = get_ranges(rng_start, rng_end, version, [group.id], False, get_all=False).limit(2).all()
    # TODO: объединить все функции добавления!!
    if len(existing_ranges) > 1:
        raise OverlapRangeError(_("Intersecting nets found"))
    elif len(existing_ranges) == 1:
        found_rng = existing_ranges[0]
        if found_rng.rng_start != rng_start or found_rng.rng_end != rng_end:
            raise OverlapRangeError(_("Intersecting net found"))
        else:
            cbb_error(scold_on_repeat, RangeAlreadyExistsError(_("This net already exists in this blocking")))
            return
    if not group.is_internal:
        net_result = cut_internal_ranges(net_result, version)
    else:
        external_ranges = get_external_ranges(rng_start, rng_end, version)
        if external_ranges:
            return external_ranges

    # в net_result лежит порезанная сеть, куски которой могли
    # перестать быть сетью
    if not net_result:
        cbb_error(scold_on_repeat,
                  EmptyRangeError(_("This net fully contains in internal")))
        return

    for (net_start, net_end) in net_result:
        ip_start = IPAddress(net_start, version)
        ip_end = IPAddress(net_end, version)
        for net in summarize_address_range(ip_start, ip_end):
            r_start = get_ip_from_string(net.network, version)
            r_end = get_ip_from_string(net.broadcast, version)
            new_block = RangeClass(rng_start=r_start, rng_end=r_end, group_id=group.id, user=user, block_descr=block_descr)
            db.main.session.add(new_block)
    group.update_updated()


# Net exceptions actions
def add_del_netexcept(group, net_ip, net_mask, except_ip, except_mask, operation, block_descr="", operation_descr="", user="", version=4):
    if ranges_types[group.default_type] != "cidr":
        raise BadGroupTypeError(_("Wrong group type"))

    if group.is_internal:
        raise ExceptionToInternalError(_("Can not add exception to internal group"))

    (rng_start, rng_end) = get_range_from_net(net_ip, net_mask, version)
    (except_start, except_end) = get_range_from_net(except_ip, except_mask, version)

    if except_start == rng_start and except_end == rng_end:
        raise ExceptionEqualsNetError(_("Exception fully match range."))

    if except_start < rng_start or except_end > rng_end:
        raise ExceptionMismatchError(_("Given range does not include this exception"))

    RangeClass = BLOCK_VERSIONS[version]
    RangeClass.query.filter_by(rng_start=rng_start, rng_end=rng_end, group_id=group.id, is_exception=False).one()

    if operation == "add":
        exceptions = get_ranges(except_start, except_end, version, [group.id], is_exception=True)
        if exceptions:
            raise OverlapExceptionsError(_("Intersecting exceptions found"))

        exception = RangeClass(rng_start=except_start, rng_end=except_end, group_id=group.id, is_exception=True, block_descr=block_descr, user=user)
        db.main.session.add(exception)
        group.update_updated()
        user = ""
        operation_descr = ""
    elif operation == "del":
        exception = RangeClass.query.filter_by(rng_start=except_start, rng_end=except_end, group_id=group.id, is_exception=True).one()
        db.main.session.delete(exception)

        HISTORY_VERSIONS[version].add(block=exception, group=group, unblocked_by=user, unblock_description=operation_descr)
    else:
        raise WrongOperationError(_("Wrong operation"))
