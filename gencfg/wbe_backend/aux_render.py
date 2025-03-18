def shrink_xinterval(old_xcoords, points):
    if len(old_xcoords) <= points:
        return old_xcoords

    left, right = old_xcoords[0], old_xcoords[-1]
    new_xcoords = []
    for i in range(points):
        new_xcoords.append(left + (right - left) * i / (points - 1))

    return new_xcoords


def shrink_xcoords(xcoords):
    """
        This functions shrinks xcoords by reducing number of points for older dates
        params:
            - xcoords: list of int timestamps since epoch

        returns: shrinked list of int timestampes since epoch
    """

    if len(xcoords) == 0:
        return xcoords

    SHRINK_SECTIONS = [
        (168, 3600),  # Last week one point per hour
        (180, 14400),  # Last month one point per 4 hours
        (730, 86400),  # Last two years one point per day
    ]

    new_xcoords = []

    endx = xcoords[-1]
    for points, interval in SHRINK_SECTIONS:
        startx = endx - points * interval

        affected_points = filter(lambda x: startx < x <= endx, xcoords)

        new_xcoords += shrink_xinterval(affected_points, points)

        endx = startx

    return new_xcoords


def shrink_ycoords(old_xcoords, old_ycoords, new_xcoords=None):
    if new_xcoords is None:
        new_xcoords = shrink_xcoords(old_xcoords)

    #    assert new_xcoords[0] == old_xcoords[0]

    new_ycoords = [old_ycoords[0]]

    old_xcoord_index = 0
    for new_xcoord in new_xcoords[1:]:
        ind = old_xcoord_index
        while ind + 1 < len(old_xcoords) and old_xcoords[ind + 1] <= new_xcoord:
            ind += 1

        if ind == old_xcoord_index:
            new_ycoords.append(new_ycoords[-1])
        else:
            lst = old_ycoords[old_xcoord_index + 1:ind + 1]
            new_ycoords.append(sum(lst) / len(lst))

        old_xcoord_index = ind

    return new_ycoords
