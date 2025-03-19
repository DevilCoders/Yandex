# ssh keys for scp from backends and trconv

/var/www/.ssh:
  file.directory:
    - makedirs: true
    - user: www-data
    - group: www-data
    - dir_mode: 700
    - file_mode: 600
    - recurse:
      - user
      - group
      - mode


ssh_keys:
  ssh_auth.present:
    - user: www-data
    - enc: ssh-rsa
    - names:
      - AAAAB3NzaC1yc2EAAAABIwAAAQEAtoO3EGxr8Qaq5410LR3gxRCWVcwu9I41WovAZZcXn5El+ggQKDVk+1wFd1T3xIqcI5DtfDc+ZVEjQSPtn6VxSk63zqlYomwRoFmYFn1hu70t4vdRGccvCxGtyRgua3qm0Dp4eBLqXSVGGKva0FxGzsBZ9Q3gxNV7XJjdZpLzs086N+zaRyS48VTI4oOQ7X0URWH7sAzKEuDTYyHwumWxsPCnWOXeN6AS4fFXF9kSC7Z/F8FvPDQ+Lmmtq18NQC/o7qVlyCV+kbDI36jDWq64+cwV7rFJMbW3VeFBKXB3eZziwbnpoc1DLDuYKIuyIax7pc6pf7SMA39mRxJe29qOjQ== www-data@trconv.kinopoisk.ru
      - AAAAB3NzaC1yc2EAAAADAQABAAABAQCpjHmPBrovTm1peuyXxVBwwg6Zrqhza/33L6sl52trzmwRgC5TTq/o8kO8E0s9Zx+7HlGP7zPNxyh5EtNScJUpzSoL6lJ/ERmIfgUBi7F9HneA+blFLiDwFWeYMNcBvcw5fZG4rTF3zkh4GitGGcyhB5tXVUcBSz2LYyHcnSYqJxSDjS6jjZncGEilQ2SRLz3YOGj8gN/aMcl1sWaVzlJ3OZqMgAu6KQqNgW7ooC4SXhqoVbVMAv5IOAtkKhgYwzhzkHpRnyTGgjFjxRXqpvtdx/DoxuUMiQdqm6fuMteCINYrgVqJfYVDoISwnsnjHLrG6QX4B7ZZRKqmzm83TPjf www-data@bk01h.kp.yandex.net
