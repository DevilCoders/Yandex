from core.objects.user import User

class SK_User(User):
    rank = 4  # default - regular support
    is_absent = 0  # default - no absence
    u_id = -1       # no uid yet

    def checkout_rank(self):
        sql = f'''
        select `rank` from s_keeper_support_unit where login like '%{self.staff_login}%';
        '''
        rank = self._db_session().query(sql=sql)['rank']
        self.rank = rank
        return rank

    def checkout_absence(self):
        sql = f'''
                select is_absent from s_keeper_support_unit where login like '%{self.staff_login}%';
                '''
        is_absent = self._db_session().query(sql=sql)['is_absent']
        self.is_absent = is_absent
        return is_absent

    def checkout_uid(self):
        sql = f'''
                        select u_id from s_keeper_support_unit where login like '%{self.staff_login}%';
                        '''
        uid = self._db_session().query(sql=sql)['u_id']
        self.u_id = uid
        return uid

    def set_absent(self, is_absent):
        sql = f"""
        update s_keeper_support_unit set is_absent={is_absent} where login like '%{self.staff_login}%';
        """
        ret = self._db_session().query(sql=sql)
        self.is_absent = is_absent
        print(f'absence is set to {is_absent} for {self.staff_login}')
        return ret
