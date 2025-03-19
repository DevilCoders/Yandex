$script = @@
def money_return(p, base_money = 500, only_non_healthy = True):
    if (p is None):
        return 0
    return p * base_money
@@;
$money_return = Python3::money_return(
    Callable<(Double?,Uint64, Bool)->Double>,
    $script
); 

DEFINE subquery $help_money() AS (
    Select 
        Name,
        $money_return((CAST(Age AS Double)), 1000, True) as HelpMoney
    From  hahn.`home/cloud_analytics/tmp/mas678/test_table`
);
END DEFINE;
EXPORT $money_return, $help_money;