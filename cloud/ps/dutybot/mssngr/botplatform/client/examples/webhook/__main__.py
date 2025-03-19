import os
import logging

from aiohttp import web

from mssngr.botplatform.client.src import Bot, Message


logging.basicConfig(level=logging.DEBUG)

HOST = os.environ["HOST"]
TOKEN = os.environ["TOKEN"]
PORT = os.environ.get("PORT", 8081)
bot = Bot(HOST, TOKEN, is_team=True)


@bot.message_handler(content_type=['text', 'sticker'])
def text(message: Message):
    if message.text:
        bot.send_message(message.text, chat_id=message.chat.id)
    elif message.sticker:
        bot.send_sticker(message.sticker, chat_id=message.chat.id)


@bot.message_handler(content_type=['gallery'])
def gallery(message: Message):
    bot.send_gallery(message.gallery, text=message.text, chat_id=message.chat.id)


async def handler(request):
    try:
        data = await request.json()
        bot.serve_message(Message.from_dict(data))
        return web.Response()
    except Exception as e:
        logging.error(e)
        return web.Response(status=400, text=str(e))


app = web.Application()
app.router.add_post('/echo', handler)

web.run_app(app, host='::', port=PORT)
