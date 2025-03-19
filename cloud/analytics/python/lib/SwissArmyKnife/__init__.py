#!usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Thu Nov  1 14:55:25 2018

@author: artkaz
"""

##test commit from VS Code ggggg 53454

from nile.api.v1 import clusters, Record
import pandas as pd
from datetime import datetime, date, timedelta
import requests
from StringIO import StringIO
import os

#%%


sw_folder = os.path.dirname(os.path.realpath(__file__))


class SwissArmyKnife(object):

    @staticmethod
    def d_from_ts(d, hours_shift = 3):
        try:
            return datetime.fromtimestamp(d + hours_shift * 3600).date()
        except:
            return d

    @staticmethod
    def dt_to_ts(dt):
        return time.mktime(dt.timetuple())

    @staticmethod
    def to_str_list(r):
    #print (r)
        if len(r) < 2:
            return str(r.values[0])
        else:
			r_unique = list(set(r.values))
			res=''
			for r_ in r_unique:
			    res += str(r_) + ', '
			return res[:-2]

    @staticmethod
    def first_not_nan(c):
        for c_ in c:
            if not pd.isnull(c_):
                return c_

    @staticmethod
    def d_from_str(d,pattern='%Y-%m-%d'):
        return datetime.strptime(d, pattern)
    @staticmethod
    def return_week(d):
        #d = dt.date()
        monday = d - timedelta(days=d.weekday())
        sunday = d + timedelta(days=6-d.weekday())
        return monday.strftime('%m-%d') + ' to ' + sunday.strftime('%m-%d')

    @staticmethod
    def last_monday_before(d):
        try:
            return d - timedelta(days=d.weekday())
        except:
            return d



#   предполагается, что дан DataFrame df следующей структуры: для каждого объекта (id_field),
#   принадлежащего к какой-то группе (tag_field) даны даты наступления какого-то события Х (date_field).
#   функция возвращает кумулятивное количество объектов, для которых событие X уже наступило
#   для каждой даты в диапазоне от date_start до date_end.
#   в результате получится df c двумя колонками - ['date', prefix+'count']
    @staticmethod
    def calc_vintages(df,
                      id_field,
                      date_field,
                      tag_field,
                      date_start,
                      date_end,
                      prefix
                      ):

#       1.считаем количества на каждую дату
        df_grouped = df.groupby([tag_field, date_field],
                                as_index=False).\
                                agg({id_field:'nunique'}).rename({id_field:prefix + 'count'}, axis=1)
#       2.получаем все комбинации дат и тэгов
        date_range = pd.DataFrame([date_start + timedelta(days=x) for x in range(0, (date_end-date_start).days+1)], columns=[date_field])
        date_range['a']=1
        tags = df_grouped[[tag_field]].drop_duplicates()
        tags['a']=1
        date_range_with_tags = pd.merge(date_range, tags, how='outer', on='a').drop('a', axis=1)

#       3.мерджим 1. и 2.
        df_grouped = pd.merge(date_range_with_tags, df_grouped, how='left', on=[date_field, tag_field])
        df_grouped[prefix + 'count'] = df_grouped[prefix + 'count'].fillna(0)

#       сортируем и считаем кум сумму
        df_grouped = df_grouped.sort_values(by=[tag_field, date_field])

        for tag in tags[tag_field].values:
            df_grouped.loc[df_grouped[tag_field] == tag, prefix + 'cum_count'] = df_grouped.loc[df_grouped[tag_field] == tag, prefix + 'count'].cumsum()

        return df_grouped.rename({date_field:'date'}, axis=1).drop(prefix + 'count', axis=1)




class ClickHouseClient(object):
    def __init__(self,
                 host,
                 user,
                 passw,
                 verify,
                 database):

        self.user = user
        self.passw = passw
        self.host = host
        self.verify = verify
        self.database = database

    def exec_sql(self, q):
        r = requests.post(self.host,
                          auth=(self.user, self.passw),
                          timeout = 1500,
                          data=q,
                          verify=self.verify,
                          database=self.database)
        if r.status_code == 200:
            return r.text
        else:
            raise Exception(r.text)

    def write_df_to_table(self, df, table, overwrite=True):
        s = StringIO()
        df.to_csv(s, index=False)
        df_csv = s.getvalue()[s.getvalue().find('\n') + 1:]
        df_cols = s.getvalue()[:s.getvalue().find('\n')]
        self.df_insert_query = 'INSERT INTO ' + \
                            table + \
                            ' (' + \
                            df_cols + \
                            ') FORMAT CSV ' + \
                            df_csv
        if overwrite:
            try:
                self.exec_sql('DROP TABLE ' + table)
            except:
                pass
            with open(sw_folder + '/../../../ch_tables/'+ table + '.sql', 'r') as f:
                self.create_table_query = f.read()
            self.exec_sql(self.create_table_query)

        self.exec_sql(self.df_insert_query)


class YTClient(object):
    def __init__(self, cluster_name):
        if cluster_name == 'Hahn':
            self.cluster = clusters.Hahn()

    def read_yt_table(self, yt_path):
        records = self.cluster.read(yt_path)
        return pd.DataFrame.from_dict([rec.to_dict() for rec in records] )

    def write_to_yt(self, yt_path, df):
        records = [Record.from_dict(rec) for rec in df.to_dict(orient='records')]
        self.cluster.write(yt_path, records)



if __name__ == '__main__':
    pass
