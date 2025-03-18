import datetime

from devtools.fleur.ytest import suite, test, TestSuite, generator
from devtools.fleur.ytest import AssertEqual

from antirobot.scripts.utils import flash_uid

@suite(package="antirobot.scripts.utils")
class Fuid(TestSuite):
    @generator([
        ("5497ba437e46d244.tq8tZ1djzKrnE3622EzdoFAfb5WK2e9fnIGA06aUvq7D_8A2Hzlm8FpmHprQqzOduP6Ze7QYcZ9FBqiaDxQrsU1-3jsDlkFKf5lbby7VahmbFqZEhxJov8YwhmOgSSbp"
         ,9099191288067504707
         ,1419229763
        ),
        ("547614604dc95e20.sjDR4wAozBrQkidbcEqQeqWMFuxMV-EF7avqSrrl1jqjaS3j8UPwpy_JlP87xUGSe_21dQVLjySFJjIKPcIXGiQ84Gge8iW9RGinLOQ8e1MdyqwRATow8B_FWbrn4Zt9"
         ,5605114704188281952
         ,1417024608
        )
    ])
    def Parse(self, cookVal, id, timeStamp):
        fuid = flash_uid.Fuid.FromCookieValue(cookVal)
        AssertEqual(fuid.Id, id)
        AssertEqual(fuid.Timestamp, timeStamp)

    @generator([
        (
         9099191288067504707
         ,1419229763
        ),
        (
         5605114704188281952
         ,1417024608
        )
    ])
    def FromId(self, id, timeStamp):
        fuid = flash_uid.Fuid.FromId(id)
        AssertEqual(fuid.Timestamp, timeStamp)
