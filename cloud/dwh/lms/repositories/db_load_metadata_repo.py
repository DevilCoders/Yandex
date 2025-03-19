from cloud.dwh.lms.exceptions import InvalidObjectIdError


class DBLoadMetadataRepository:
    @staticmethod
    def get_increment_value(cursor, object_id: str):
        """
        Get current load increment for an object_id
        :param cursor:
        :param object_id: str
        :return: object
        """
        cursor.execute(
            """
            select increment_value
              from meta.lms_ctrl_increment_value
             where object_id = %s
            """, (object_id, )
        )
        inc = cursor.fetchone()
        if inc is None:
            raise InvalidObjectIdError(f"object_id {object_id} does not exist")
        return inc[0]

    @staticmethod
    def update_increment_value(cursor, object_id: str, increment_value: str):
        """
        Update current load increment for an object_id
        :param cursor:
        :param increment_value:
        :param object_id: str
        :return: None
        """
        cursor.execute(
            """
            update meta.lms_ctrl_increment_value
               set increment_value = %s
             where object_id = %s
            """, (increment_value, object_id)
        )
