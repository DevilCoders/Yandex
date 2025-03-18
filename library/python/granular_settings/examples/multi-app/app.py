from app1.settings import NAME as NAME1
from app2.settings import NAME as NAME2

if __name__=="__main__":
    assert NAME1=='app1'
    assert NAME2=='app2'