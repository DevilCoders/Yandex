import os
import logging

from mssngr.botplatform.client.src.bot import Bot, Message


logging.basicConfig(level=logging.DEBUG)

HOST = os.environ["HOST"]
TOKEN = os.environ["TOKEN"]
bot = Bot(HOST, TOKEN, is_team=True)


@bot.message_handler(content_type=['document'])
def doc(message: Message):
    bot.send_document(message.document, chat_id=message.chat.id)


@bot.message_handler(content_type=['photo'])
def photo(message: Message):
    bot.send_photo(message.photo, chat_id=message.chat.id)


@bot.message_handler(content_type=['gallery'])
def gallery(message: Message):
    bot.send_gallery(message.gallery, text=message.text, chat_id=message.chat.id)


@bot.message_handler(content_type=['text'])
def text(message: Message):
    bot.send_message(message.text, chat_id=message.chat.id)


@bot.message_handler(content_type=['sticker'])
def sticker(message: Message):
    bot.send_sticker(message.sticker, chat_id=message.chat.id)


if __name__ == '__main__':
    bot.polling()
