from datetime import datetime, timedelta


def get_last_day_of_month(date_str: str) -> str:
    """Finds last date of month for unix-formatted (%Y-%m-%d) string and return unix-formatted string"""
    unix_fmt = '%Y-%m-%d'
    date_dt = datetime.strptime(date_str, unix_fmt)
    first_date_of_next_month_dt = (date_dt.replace(day=28) + timedelta(days=4)).replace(day=1)
    last_date_of_current_month_dt = first_date_of_next_month_dt - timedelta(days=1)
    last_date_of_current_month_str = last_date_of_current_month_dt.strftime(unix_fmt)
    return last_date_of_current_month_str
