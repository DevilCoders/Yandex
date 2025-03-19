import requests
import json


class Clickhouse(object):
  def __init__(self, hostname, user, password):
    self.hostname = hostname
    self.user = user
    self.password = password
    self.session = None
    self.url = "http://{}:8123/?".format(self.hostname)
    self.timeout = 1
    self.established = False
    self.query = ''
    self.answer = {}

  def make_connection(self):
    self.session = requests.Session()
    self.session.auth = (self.user, self.password)
    self.session.headers.update({'user-agent': 'https://nda.ya.ru/3SWqnc'})
    try:
      response = self.session.get(url=self.url, timeout=self.timeout)
    except requests.ConnectionError as error:
      raise error
    if response:
      if response.status_code == 200:
        self.established = True
      return self

  def send_request(self, query):
    self.query = query
    self.make_connection()
    if self.established:
      response = None
      try:
        response = self.session.post(self.url, data=self.query, timeout=self.timeout)
      except requests.ConnectionError as error:
        self.answer = {'code': 110, 'message': 'No response from server', 'error': error}
      if response:
        if response.status_code == 200:
          if response.text:
            text = None
            try:
              text = json.loads(response.text)
            except ValueError as error:  # json.loads in python2 raises ValueError
              self.answer = {'code': 200, 'message': response.text, 'error': str(error)}
            if text:
              self.answer = {'code': response.status_code, 'message': text, 'error': ''}
        else:
          self.answer = {'code': response.status_code, 'message': 'Non 200 reply', 'error': "BadResponse"}
    else:
      self.answer = {'code': 110, 'message': "No response from server", 'error': "ConnectionTimedOut"}
    return self

  def __getitem__(self, item):
    return self.__dict__.get(item)


class ClickhouseRead(Clickhouse):
  def __init__(self, hostname, database, user, password):
    self.hostname = hostname
    self.database = database
    self.user = user
    self.password = password
    super(ClickhouseRead, self).__init__(self.hostname, self.user, self.password)

  def select(self, table, fields, tail):
    q = 'SELECT {} from {} {} FORMAT JSON;'.format(fields, table, tail)
    self.send_request(q)
    return self.answer


class ClickhouseWrite(Clickhouse):
  def __init__(self, hostname, database, user, password):
    self.user = user
    self.password = password
    self.hostname = hostname
    self.database = database
    super(ClickhouseWrite, self).__init__(self.hostname, self.user, self.password)

  def insert(self, table, data):
    self.make_connection()
    data = ["\'{}\'".format(str(el)) if isinstance(el, unicode) else str(el) for el in data]
    query = "INSERT INTO {}.{} VALUES ({});".format(self.database, table, ', '.join(data))
    self.send_request(query)
    return self.answer

  def __getitem__(self, item):
    return self.__dict__.get(item)

