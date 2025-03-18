def get_worst_tier_by_size(s, mydb):
    tiers_data = map(lambda x: (x.split(':')[0], int(x.split(':')[1])), s.split(','))
    return max(map(lambda (tier, ss): mydb.tiers.tiers[tier].disk_size / ss, tiers_data))

#
#
#
#
#
#
#
#
#
#
#
#
