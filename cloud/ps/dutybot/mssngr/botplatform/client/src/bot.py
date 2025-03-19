import os
import typing as t  # noqa
import uuid
import json

import six  # noqa
import io

import requests
from PIL import Image  # type: ignore

from .dispatcher import Dispatcher
from .types import Update, Chat, WebhookInfo, Message, File, Photo  # noqa
from .exceptions import NotFoundError, ServerError, AuthError, BadRequestError


class Bot(Dispatcher):
    def __init__(
        self,
        host,  # type: six.text_type
        token,  # type: six.text_type
        is_team=True,  # type: bool
        sleep_time=1.0,  # type: float
    ):
        super(Bot, self).__init__(sleep_time)
        self._token = token  # type: six.text_type
        self._host = host  # type: six.text_type
        self._is_team = is_team  # type: bool
        self._nickname = ""  # type: six.text_type
        self._guid = ""  # type: six.text_type
        self._display_name = ""  # type: six.text_type
        self._description = None  # type: t.Optional[six.text_type]
        self._webhook_url = None  # type: t.Optional[six.text_type]
        self._session = requests.Session()  # type: requests.Session
        self.get_me()

    def get_me(self):
        # type: () -> None
        """
        Update nickname, guid, display_name, description, webhook_url fields
        """
        data = self._make_request("GET", "/bot/getMe/").json()
        self._nickname = data["nickname"]
        self._display_name = data["display_name"]
        self._guid = data["guid"]
        self._description = data["description"]
        self._webhook_url = data["webhook_url"]

    def get_chats(self):
        # type: () -> t.List[Chat]
        response = self._make_request("GET", "/bot/getChats/").json()
        return [Chat.from_dict(c) for c in response]

    def get_updates(self, offset=0):
        # type: (int) -> t.List[Update]
        response = self._make_request("POST", "/bot/telegram_lite/getUpdates/", data={"offset": offset}).json()
        return [Update.from_dict(u) for u in response]

    def download_file(
        self,
        file_obj,  # type: File
        destination,  # type: t.Any
        seek=True,  # type: bool
        chunk_size=65536,  # type: int
    ):
        # type: (...) -> t.Any
        """
        Write file content into destination or crete new file if destination is string
        :param file_obj:  File object
        :param destination:  None or directory or path to file or instance of io.IOBase
        :param seek: go to start of file when downloading is finished if True
        :param chunk_size:
        :return: IOBase
        """
        if destination is None:
            destination = file_obj.name

        elif isinstance(destination, six.text_type):
            if os.path.isdir(destination):
                destination = os.path.join(destination, file_obj.name)

        dest = self._check_path(destination, "wb")

        url = "{}/bot/{}".format(self._host, file_obj.file_id)
        headers = {"Authorization": self._auth_token()}

        with self._do_request_with_retries("GET", url, attempts=3, stream=True, headers=headers) as resp:
            self._validate_response(resp)
            for chunk in resp.iter_content(chunk_size=chunk_size):
                dest.write(chunk)
            dest.flush()
        if seek:
            dest.seek(0)
        return dest

    def send_seen_marker(self, message_id, **kwargs):
        # type: (int, **six.text_type) -> None
        """

        :param message_id:
        :param kwargs: chat_id, user_uid or user_login should be set
        :return:
        """
        data = {
            "message_id": message_id,
        }
        self._make_request(
            "POST",
            "/bot/telegram/sendSeenMarker/",
            data=data,
            require_destination_id=True,
            **kwargs
        )

    def send_typing(self, **kwargs):
        # type: (**six.text_type) -> None
        """
        :param kwargs: chat_id, user_uid or user_login should be set
        :return:
        """
        self._make_request(
            "POST",
            "/bot/telegram_lite/sendTyping/",
            data={},
            require_destination_id=True,
            **kwargs
        )

    def send_heartbeat(self, online_until=180):
        # type: (int) -> None
        """
        The time during which the bot should be counted online, in seconds
        """
        self._make_request(
            "POST",
            "/bot/telegram_lite/sendHeartbeat/",
            data={
                "online_until": online_until,
            },
        )

    def send_message(
        self,
        text=None,  # type: t.Optional[six.text_type]
        message_id=None,  # type: t.Optional[int]
        reply_to_message_id=None,  # type: t.Optional[int]
        notification=None,  # type: t.Optional[six.text_type]
        disable_web_page_preview=None,  # type: t.Optional[bool]
        important=None,  # type: t.Optional[bool]
        reply_markup=None,  # type: t.Optional[t.List[t.List[t.Dict[six.text_type, t.Union[six.text_type, bool]]]]]
        card=None,  # type: t.Optional[t.Dict[six.text_type, t.Any]]
        **kwargs  # type: t.Any
    ):  # type: (...) -> Message
        """
        :param text: required
        :param message_id: id of the message being edited
        :param reply_to_message_id: id of the message on what you want create reply
        :param notification: text that will be in the notification (not sure that it works)
        :param disable_web_page_preview: disable preview of the page (not sure that it works)
        :param important: mark the message as important
        :param reply_markup: list of dicts with following fields. Only url or callback_data should be set
            {
                "text": "text that will be set on button",
                "url": "this url will be opened after click", // !!doesn't work in web and android (the button is not displayed)
                "callback_data": 4kb of text, that will be sent after click, // !!doesn't work in web
                "hide": hide button after click
            }
        :param card: doesn't validate format. Format should be like here https://bp.mssngr.yandex.net/docs/api/bot/types/#card
               Card doc see https://wiki.yandex-team.ru/assistant/alicekit/divnaja-dokumentacija/.
        """
        data = {}  # type: t.Dict[str, t.Any]

        if text:
            data["text"] = text
        if message_id:
            data["message_id"] = message_id
        if not text and not message_id:
            raise ValueError('Text or message_id should be set')

        if reply_to_message_id:
            data["reply_to_message_id"] = reply_to_message_id
        if disable_web_page_preview is not None:
            data["disable_web_page_preview"] = disable_web_page_preview
        if important is not None:
            data["important"] = important
        if notification:
            data["notification"] = notification
        if reply_markup:
            ### andgein: this code is wrong. See https://st.yandex-team.ru/MSSNGRBACKEND-1032
            ### and https://bp.mssngr.yandex.net/docs/api/bot/types/#inlinekeyboardmarkup.
            ### `reply_markup` should be array of array of buttons
            # for btn in reply_markup:
            #     if 'callback_data' in btn and 'url' in btn:
            #         raise ValueError('Only url or callback_data should be set in {}'.format(btn))
            #     if 'callback_data' not in btn and 'url' not in btn:
            #         raise ValueError('Url or callback_data should be set in {}'.format(btn))
            data["reply_markup"] = {"inline_keyboard": reply_markup}
        if card:
            data["card"] = card

        resp = self._make_request(
            "POST",
            "/bot/telegram_lite/{}Message/".format("edit" if message_id else "send"),
            json=data,
            require_destination_id=True,
            **kwargs
        ).json()
        return Message.from_dict(resp["message"])

    def edit_message(self, message_id, **kwargs):
        # type: (...) -> Message
        """
        :param message_id: id of the message being edited
        :param kwargs: the same parameters as for send_message method
        """
        return self.send_message(message_id=message_id, **kwargs)

    @staticmethod
    def _check_path(path, mode="rb"):
        # type: (t.Any, six.text_type) -> t.Any
        # file is a base class for files in python2, but in python3 IOBase is a base class
        if six.PY2:
            path = path if isinstance(path, (file, six.BytesIO, six.StringIO)) else open(path, mode)  # noqa
        else:
            path = path if isinstance(path, (io.IOBase, six.BytesIO, six.StringIO)) else open(path, mode)
        return path

    def _upload_file(
        self,
        source,  # type: t.Any
        name,  # type: t.Optional[six.text_type]
        **kwargs  # type: t.Any
    ):  # type: (...) -> requests.Response

        source = self._check_path(source)
        name = name if name else os.path.split(source.name)[-1]
        data = {
            "filename": name,
        }
        files = {
            "document": source,
        }
        return self._make_request(
            "POST",
            "/bot/telegram_lite/sendDocument/",
            data=data,
            files=files,
            require_destination_id=True,
            **kwargs
        )

    def send_document(
        self,
        file,  # type: t.Any
        name=None,  # type: t.Optional[six.text_type]
        **kwargs  # type: t.Any
    ):  # type: (...) -> Message
        """
        :param file:
        :param name: if name is None or empty string, it will be replaced by file name
        :param kwargs: chat_id, user_uid or user_login should be set
        :return:
        """
        if isinstance(file, File):
            data = file.to_dict()
            resp = self._make_request(
                "POST",
                "/bot/telegram_lite/sendDocument/",
                data=data,
                require_destination_id=True,
                **kwargs
            )
        else:
            resp = self._upload_file(file, name, **kwargs)

        return Message.from_dict(resp.json()["message"])

    def _prepare_photo(
        self,
        source,  # type: t.Any
        name,  # type: t.Optional[six.text_type]
    ):  # type: (...) -> t.Tuple[t.Dict[six.text_type, six.text_type], t.Dict[six.text_type, t.Any], t.Dict[six.text_type, t.Any]]
        source = self._check_path(source)
        name = name if name else os.path.split(source.name)[-1]

        img = Image.open(source)
        width, height = img.size
        source.seek(0)

        data = {
            "filename": name or 'photo',
        }

        files = {
            "photo": source,
        }
        photo_info = {
            "file_name": data["filename"],
            "size": os.path.getsize(files["photo"].name),
            "width": width, "height": height
        }
        return data, files, photo_info

    def _upload_photo(
        self,
        source,  # type: t.Any
        name,  # type: t.Optional[six.text_type]
        **kwargs  # type: t.Any
    ):  # type: (...) -> requests.Response

        data, files, photo_info = self._prepare_photo(source, name)
        resp = self._make_request(
            "POST",
            "/bot/telegram_lite/sendPhoto/",
            data=data,
            files=files,
            require_destination_id=True,
            **kwargs
        )
        return resp

    def send_photo(
        self,
        photo,  # type: t.Any
        name=None,  # type: t.Optional[six.text_type]
        **kwargs  # type: t.Any
    ):
        # type: (...) -> Message
        """
        :param photo: photo, path or file object
        :param name: this name will be assign to the file
        :param kwargs: chat_id, user_uid or user_login should be set
        :return:
        """
        if isinstance(photo, Photo):
            data = photo.to_dict()
            data.update({
                "filename": photo.name or 'photo',
            })
            resp = self._make_request(
                "POST",
                "/bot/telegram_lite/sendPhoto/",
                data=data,
                require_destination_id=True,
                **kwargs
            )
        else:
            resp = self._upload_photo(photo, name, **kwargs)

        return Message.from_dict(resp.json()["message"])

    def send_gallery(
        self,
        gallery=None,  # type: t.Optional[Gallery]
        files=None,    # type: t.List[t.Any]
        text="",  # type: t.Optional[six.text_type]
        **kwargs  # type: t.Any
    ):  # type: (...) -> Message
        """
        :param gallery: gallery object
        :param files: list of file names of file objects
        :param text:
        :param kwargs: chat_id, user_uid or user_login should be set
        :return:
        """
        result = []  # type: t.List[t.Any]
        data = {
            "text": text,
            "items": [],
        }  # type: t.Dict[six.text_type, t.Any]

        if not gallery and not files:
            raise ValueError('please specify gallery or files')

        if gallery:
            for p in gallery.photos:
                img = p.to_dict()
                # TODO: unify and fix
                img["filename"] = p.name or "photo"
                img["file_name"] = p.name or "photo"
                data["items"].append({"image": img})
                result.append(img)

        files_ = []
        if files:
            for p in files:
                _, f, photo_info = self._prepare_photo(p, "photo")
                files_.append(("photo", f["photo"]))
                result.append(photo_info)

        # request fails with 400 (no chat_id was set) if json was send with non empty files
        # if files not empty send data in formdata, not json
        # if items set in formdata, request will fail because backend doesn't parse items as json
        request_params = {}
        if files_:
            request_params["data"] = data
        else:
            request_params["json"] = data

        request_params.update(kwargs)
        resp = self._make_request(
            "POST",
            "/bot/telegram_lite/sendGallery/",
            files=files_,
            require_destination_id=True,
            **request_params
        ).json()

        for i in range(len(result)):
            result[i].update(resp["message"]["photos"][i][-1])
            result[i] = [result[i]]

        resp["message"]["photos"] = result

        return Message.from_dict(resp["message"])

    def send_sticker(self, sticker, **kwargs):
        # type: (Sticker, **six.text_type) -> Message
        """
        :param set_id:
        :param sticker_id:
        :param kwargs: chat_id, user_uid or user_login should be set
        :return:
        """
        data = {
            "set_id": sticker.set_id,
            "sticker_id": sticker.sticker_id,
        }
        resp = self._make_request(
            "POST", "/bot/telegram_lite/sendSticker/",
            data=data,
            require_destination_id=True,
            **kwargs
        ).json()

        return Message.from_dict(resp["message"])

    def get_webhook_info(self):
        # type: () -> WebhookInfo
        response = self._make_request("GET", '/bot/telegram_lite/getWebhookInfo/').json()
        return WebhookInfo.from_dict(response)

    def _do_request_with_retries(self, method, url, attempts, **kwargs):
        # type: (six.text_type, six.text_type, int, **t.Any) -> requests.Response
        resp = requests.Response()
        while attempts != 0:
            resp = self._session.request(method, url, **kwargs)
            attempts -= 1
            if resp.status_code < 500:
                return resp
        return resp

    def _make_request(self, method, url, require_destination_id=False, **kwargs):
        # type: (six.text_type, six.text_type, bool, **t.Any) -> requests.Response
        if not url.startswith('/'):
            url = '/' + url
        if not url.endswith('/'):
            url += '/'

        data_field = None
        if "data" in kwargs:
            data_field = "data"
        elif "json" in kwargs:
            data_field = "json"

        if data_field and require_destination_id:
            keys = ['chat_id', 'guid', 'uid', 'user_uid', 'login', 'user_login']

            dest_id = None
            for k in keys:
                if k in kwargs:
                    dest_id = k
                    break

            if not dest_id:
                raise ValueError('none of arguments {} were set'.format(', '.join(keys)))
            kwargs[data_field][dest_id] = kwargs[dest_id]
            del kwargs[dest_id]

        if data_field:
            if 'payload_id' in kwargs:
                kwargs[data_field]['payload_id'] = kwargs['payload_id']
            else:
                kwargs[data_field]['payload_id'] = str(uuid.uuid4())

        url = "{}{}".format(self._host, url)
        if not kwargs.get("headers"):
            kwargs["headers"] = {}
        kwargs["headers"].update({
            "Authorization": self._auth_token()
        })

        resp = self._do_request_with_retries(method, url, attempts=3, **kwargs)

        self._validate_response(resp)
        return resp

    def _auth_token(self):
        # type: () -> six.text_type
        return "{} {}".format(
            "OAuthTeam" if self._is_team else "OAuth",
            self._token,
        )

    @staticmethod
    def _validate_response(response):
        # type: (requests.Response) -> None
        if 200 <= response.status_code < 300:
            return
        try:
            # error without description returned (can't parse json)
            err = response.json().get("detail") or response.json().get("error") or ""
        except json.JSONDecodeError:
            if response.status_code == 404:
                raise NotFoundError()
            else:
                raise ServerError(code=response.status_code)

        if response.status_code == 400:
            raise BadRequestError(err)
        elif response.status_code == 404:
            raise NotFoundError(err)
        elif response.status_code in (401, 403):
            raise AuthError(err, code=response.status_code)
        elif response.status_code >= 500:
            raise ServerError(err, code=response.status_code)
        else:
            raise NotImplementedError()

    @property
    def token(self):  # type: () -> six.text_type
        return self._token

    @property
    def host(self):  # type: () -> six.text_type
        return self._host

    @property
    def is_team(self):  # type: () -> bool
        return self._is_team

    @property
    def nickname(self):  # type: () -> six.text_type
        return self._nickname

    @property
    def guid(self):  # type: () -> six.text_type
        return self._guid

    @property
    def display_name(self):  # type: () -> six.text_type
        return self._display_name

    @property
    def description(self):  # type: () -> t.Optional[six.text_type]
        return self._description

    @property
    def webhook_url(self):  # type: () -> t.Optional[six.text_type]
        return self._webhook_url
