from datetime import datetime

from sqlalchemy import Column, DateTime, Integer, String, Text


class BlockPrototype(object):
    """
    Prototype for all blocks
    """
    group_id = Column(Integer,  default=0, nullable=False, index=True)
    created = Column(DateTime, default=datetime.now, nullable=False)
    block_descr = Column(Text, default="", nullable=False)
    expire = Column(DateTime, nullable=True, index=True)
    user = Column(String(46), default="", nullable=False)

    @classmethod
    def get_by_groups(cls, groups):
        # ToDo: add validators
        if groups:
            query = cls.query.filter(cls.group_id.in_(groups))
        return query
