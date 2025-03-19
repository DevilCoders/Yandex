from datetime import timedelta
from dateutil.parser import parse
from datetime import datetime, timedelta

def daterange(start_date, end_date):
    for n in range(int ((end_date - start_date).days / 7 + 1) ):
        yield start_date + timedelta(n * 7)

end_date = datetime.now()
start_train_date = parse('2019-06-10')
end_train_date = parse('2019-08-26')
for curr_date in daterange(start_train_date, end_train_date):
    print(curr_date.strftime('%Y-%m-%d'))
