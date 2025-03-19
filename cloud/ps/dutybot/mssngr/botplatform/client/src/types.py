import typing as t  # noqa
import json
import six  # noqa


class _BaseObject(object):
    @classmethod
    def from_json(cls, json_string):
        # type: (six.text_type) -> _BaseObject
        return cls.from_dict(json.loads(json_string))

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> _BaseObject
        raise NotImplementedError

    def to_json(self):
        # type: () -> six.text_type
        return json.dumps(self.to_dict())

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        raise NotImplementedError

    def __repr__(self):
        # type: () -> six.text_type
        return str(self)

    def __str__(self):
        # type: () -> six.text_type
        d = {}
        for k, v in six.iteritems(self.__dict__):
            if k.startswith('_'):
                k = k[1:]

            if hasattr(v, '__dict__'):
                d[k] = v.__dict__
            else:
                d[k] = v

        return six.text_type(d)


class Chat(_BaseObject):
    def __init__(self, chat_id, chat_type, title, description):
        # type: (six.text_type, six.text_type, six.text_type, t.Optional[six.text_type]) -> None
        self._id = chat_id  # type: six.text_type
        self._type = chat_type  # type: six.text_type
        self._title = title  # type: six.text_type
        self._description = description  # type: t.Optional[six.text_type] # None for private chats

    @classmethod
    def from_dict(cls, obj):  # type: (t.Dict[six.text_type, t.Any]) -> Chat
        return cls(
            chat_id=obj.get("id"),
            chat_type=obj.get("type"),
            title=obj.get("title") or obj.get("username"),
            description=obj.get("description")
        )

    def to_dict(self):  # type: () -> t.Dict[six.text_type, t.Any]
        res = {
            "id": self._id,
            "type": self._type,
        }
        if self._type == "group" and self._description is not None:
            res["title"] = self._title
            res["description"] = self._description
        else:
            res["username"] = self._title
        return res

    @property
    def id(self):  # type: () -> six.text_type
        return self._id

    @property
    def type(self):  # type: () -> six.text_type
        return self._type

    @property
    def title(self):  # type: () -> six.text_type
        return self._title

    @property
    def description(self):  # type: () -> t.Optional[six.text_type]
        return self._description


class User(_BaseObject):
    def __init__(self, is_bot, login, display_name, user_id, uid=None):
        # type: (bool, six.text_type, six.text_type, six.text_type, int) -> None
        self._is_bot = is_bot  # type: bool
        self._login = login  # type: six.text_type
        self._display_name = display_name  # type: six.text_type
        self._id = user_id  # type: six.text_type
        self._uid = uid  # type: int

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> User
        cls_dict = dict(
            is_bot=obj.get("is_bot"),
            login=obj.get("login"),
            display_name=obj.get("display_name"),
            user_id=obj.get("id"),
        )
        if obj.get("uid"):
            cls_dict["uid"] = obj["uid"]
        return cls(
            **cls_dict
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "is_bot": self._is_bot,
            "login": self._login,
            "display_name": self._display_name,
            "id": self._id,
            "uid": self._uid,
        }

    @property
    def is_bot(self):  # type: () -> bool
        return self._is_bot

    @property
    def login(self):  # type: () -> six.text_type
        return self._login

    @property
    def display_name(self):  # type: () -> six.text_type
        return self._display_name

    @property
    def id(self):  # type: () -> six.text_type
        return self._id

    @property
    def uid(self):  # type: () -> int
        return self._uid


class Entity(_BaseObject):
    def __init__(self, mention_type, length, offset):
        # type: (six.text_type, int, int) -> None
        self._type = mention_type  # type: six.text_type
        self._length = length  # type: int
        self._offset = offset  # type: int

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> Entity
        return cls(
            mention_type=obj["type"],
            length=obj["length"],
            offset=obj["offset"],
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "type": self._type,
            "length": self._length,
            "offset": self._offset,
        }

    @property
    def type(self):  # type: () -> six.text_type
        return self._type

    @property
    def length(self):  # type: () -> int
        return self._length

    @property
    def offset(self):  # type: () -> int
        return self._offset


class Sticker(_BaseObject):
    def __init__(self, set_id, sticker_id):
        # type: (six.text_type, six.text_type) -> None
        self._set_id = set_id
        self._sticker_id = sticker_id

    @classmethod
    def from_dict(cls, obj):  # type: (t.Dict[six.text_type, t.Any]) -> Sticker
        return Sticker(
            set_id=obj["set_id"],
            sticker_id=obj["file_id"],
        )

    def to_dict(self):  # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "set_id": self._set_id,
            "file_id": self._sticker_id,
        }

    @property
    def set_id(self):  # type: () -> six.text_type
        return self._set_id

    @property
    def sticker_id(self):  # type: () -> six.text_type
        return self._sticker_id


class Message(_BaseObject):
    def __init__(
        self,
        from_user,  # type: User
        chat,  # type: Chat
        date,  # type: int
        text,  # type: t.Optional[six.text_type]  # None when document, photo or sticker was sent
        message_id,  # type: int
        entities,  # type: t.List[Entity]
        reply_on=None,  # type: t.Optional[Message]
        document=None,  # type: t.Optional[File]
        photo=None,  # type: t.Optional[Photo]
        sticker=None,  # type: t.Optional[Sticker]
        gallery=None,  # type: t.Optional[Gallery]
        forwarded_messages=None,  # type: t.Optional[t.List[Message]]
    ):  # type: (...) -> None
        self._from_user = from_user
        self._chat = chat
        self._date = date
        self._text = text
        self._id = message_id
        self._entities = entities
        self._reply_to = reply_on
        self._document = document
        self._photo = photo
        self._sticker = sticker
        self._gallery = gallery
        self._forwarded_messages = forwarded_messages

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> Message
        return cls(
            from_user=User.from_dict(obj.get("from", {})),
            chat=Chat.from_dict(obj["chat"]),
            date=obj["date"],
            text=obj.get("text"),
            message_id=obj["message_id"],
            entities=[Entity.from_dict(e) for e in obj.get("entities", [])],
            reply_on=Message.from_dict(obj["reply_to_message"]) if obj.get("reply_to_message") else None,
            document=File.from_dict(obj["document"]) if obj.get("document") else None,
            photo=Photo.from_list(obj["photo"]) if obj.get("photo") else None,
            sticker=Sticker.from_dict(obj["sticker"]) if obj.get("sticker") else None,
            gallery=Gallery.from_dict(obj["photos"]) if obj.get("photos") else None,
            forwarded_messages=[Message.from_dict(m) for m in obj.get("forwarded_messages", [])],
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        d = {
            "from": self._from_user.to_dict(),
            "author": self._from_user.to_dict(),
            "chat": self._chat.to_dict(),
            "entities": [e.to_dict() for e in self._entities],
            "date": self._date,
            "text": self._text,
            "message_id": self._id,
        }
        if self._reply_to:
            d["reply_to_message"] = self._reply_to.to_dict()

        if self._sticker:
            d["sticker"] = self._sticker.to_dict()
        return d

    @property
    def from_user(self):  # type: () -> User
        return self._from_user

    @property
    def chat(self):  # type: () -> Chat
        return self._chat

    @property
    def date(self):  # type: () -> int
        return self._date

    @property
    def text(self):  # type: () -> t.Optional[six.text_type]
        return self._text

    @property
    def id(self):  # type: () -> int
        return self._id

    @property
    def entities(self):  # type: () -> t.List[Entity]
        return self._entities

    @property
    def reply_to(self):  # type: () -> t.Optional[Message]
        return self._reply_to

    @property
    def document(self):  # type: () -> t.Optional[File]
        return self._document

    @property
    def photo(self):  # type: () -> t.Optional[Photo]
        return self._photo

    @property
    def sticker(self):  # type: () -> t.Optional[Sticker]
        return self._sticker

    @property
    def gallery(self):  # type: () -> t.Optional[Gallery]
        return self._gallery

    @property
    def forwarded_messaged(self):  # type: () -> t.Optional[t.List[Message]]
        return self._forwarded_messages


class Update(_BaseObject):
    def __init__(self, message, update_id):
        # type: (Message, int) -> None
        self._message = message  # type: Message
        self._id = update_id  # type: int

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> Update
        return cls(
            Message.from_dict(obj["message"]),
            obj.get("update_id"),
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "message": self._message.to_dict(),
            "update_id": self._id,
        }

    @property
    def message(self):  # type: () -> Message
        return self._message

    @property
    def id(self):  # type: () -> int
        return self._id


class WebhookInfo(_BaseObject):
    def __init__(self, url, pending_update_count):
        # type: (six.text_type, int) -> None
        self._url = url  # type: six.text_type
        self._pending_update_count = pending_update_count  # type: int

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> WebhookInfo
        return cls(
            obj["url"],
            obj["pending_update_count"],
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "url": self._url,
            "pending_update_count": self._pending_update_count,
        }

    @property
    def url(self):  # type: () -> six.text_type
        return self._url

    @property
    def pending_update_count(self):  # type: () -> int
        return self._pending_update_count


class File(_BaseObject):
    def __init__(self, file_id, name, size):
        # type: (six.text_type, six.text_type, int) -> None
        self._file_id = file_id  # type: six.text_type
        self._name = name  # type: six.text_type
        self._size = size  # type: int

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> File
        return cls(
            obj["file_id"],
            obj["file_name"],
            obj["file_size"],
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "file_id": self._file_id,
            "filename": self._name,
            "size": self._size
        }

    @property
    def file_id(self):  # type: () -> six.text_type
        return self._file_id

    @property
    def name(self):  # type: () -> six.text_type
        return self._name

    @property
    def size(self):  # type: () -> int
        return self._size


class Photo(File):
    def __init__(self, file_id, name, size, width, height):
        # type: (six.text_type, six.text_type, int, int, int) -> None
        """photo's file name is always empty string"""
        super(Photo, self).__init__(file_id, name, size)
        self._width = width
        self._height = height

    @classmethod
    def from_list(cls, objs):
        # type: (t.List[t.Dict[six.text_type, t.Any]]) -> Photo
        # get photo of original size - the last one
        return cls.from_dict(objs[-1])

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> Photo
        return cls(
            obj["file_id"],
            obj.get("file_name", ""),
            obj["size"],
            obj["width"],
            obj["height"],
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return {
            "file_id": self._file_id,
            "filename": self._name,
            "size": self._size,
            "width": self._width,
            "height": self._height,
        }

    @property
    def width(self):
        # type: () -> int
        return self._width

    @property
    def height(self):
        # type: () -> int
        return self._height


class Gallery(_BaseObject):
    def __init__(self, photos):
        # type: (t.List[Photo]) -> None
        self._photos = photos

    @classmethod
    def from_dict(cls, obj):
        # type: (t.Dict[six.text_type, t.Any]) -> Gallery
        return cls(
            [Photo.from_list(p) for p in obj]
        )

    def to_dict(self):
        # type: () -> t.Dict[six.text_type, t.Any]
        return [p.to_json() for p in self._photos]

    @property
    def photos(self):
        # type: () -> t.List[Photo]
        return self._photos
