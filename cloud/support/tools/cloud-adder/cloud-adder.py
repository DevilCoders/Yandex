import certifi
import asyncio
from functools import partial
from typing import Tuple, Type
from tqdm.asyncio import tqdm, tqdm_asyncio
from telethon import TelegramClient
from telethon.sessions import StringSession
from telethon import errors as tgerrors
from telethon import functions as tgfunctions
import ssl
import httpx
from os import path
from pathlib import Path

chats = {'Ламповая_поддержка_Яндекс_Облака': 'SbJEQUDcchbwgWxj',
         'Свободные тикеты': 'jV25to2xY1A2ZThi',
         'Cloud_Level2_Support': 'VwlhMjgAo1g9OlFE',
         'Support_News': 'S8cbENelxXa-vTM8',
         '[NDA] YandexCloud Flood': 'TFh9oic808p8nXm8',
         'Cloud PROD Support': 'bBsqImVsbmpmZTEy',
         'CloudIL(Israel) PROD': 'Ah4l1V-zwbEzMjMy',
         'Cloud PRE & TESTING Support': 'AXmZSkKKXwhyKH-lJyOLtw',
         '[NDA] Distributed Cloud': 'DmVJURKxDFGs8Iy4yJn4ZQ',
         'YC Support': 'AXmZSh2inWsMa9oz49DTiA',
         'Support + AI': 'AXmZShOOFTllyc9FBFzjFA',
         'Cloud deploy workgroup': 'AIZQClhzwueeRBMiZPG_nw',
         'YC.Compute Team Chat': 'SJwG0Jz9P5zzRioV',
         'YC-Billing Lobby': 'DD6oP0U8pZX75k50noBq0Q',
         'YC: Marketplace-SupportAPI Support': 'pncoz3EtatxkZjJi',
         'YC Load Balancer': 'B3-v1llQaKyQaQ9v4F9o5A',
         'YandexCloud | VPC': 'D5IPyQnJj8BxLfSgZxquSw',
         'VPC | Controlfigplane Support': 'BTGXMVAr-y4YL5iLBkSebg',
         'YDB for Yandex.Cloud (NDA)': 'DmVJUQy61Ehm9ZwPOpVK5Q',
         'DBaaS Support': 'ACTM_Q1EXlpkYSYz-NchxA',
         'YCloud | Data Proc & Kafka': 'B0H1gEerJLUkW-0Vug6OQQ',
         'SQS (внутренняя инсталляция)': 'CVZG0E5ZOoq8QzWRP6OucQ',
         'YC.Bastion': 'D98VvhK7a6LxaXIRi4fEdg',
         'YC.Docs': 'TMjUNOz74kNvH1Zh',
         'YC BackOffice UI': 'AXmZShdQIPs2ncA9p3B_dw',
         'YC InstanceGroups': 'Bkq6nBLK-XB2vFOdRI3FcA',
         'yc-container-registry-support': 'DwJuixVkczL8eg5J0YfuBA',
         'YC Managed Kubernetes support': 'DsESjBOqBpH_D75SgONmgA',
         'YMQ Internal Support': 'FOacPUmaO2G7ZjGauNLZ0Q',
         'YC public API stack, self-service': 'P0uloEwF29M81BfJukEO1g',
         'hashicorp tools for Yandex.Cloud [terraform-provider-yandex & packer builder]': 'BcYyEEe8l3tbZbK0c7zRzw',
         'YC Platform L7 (ALB)': 'EdPY11UF3GccRF9-M51t0w',
         'YC: S3 + UI': 'B_i6Cw_kgfXmrl-Lr3EVLA',
         'Support S3 for YC': 'Bbsak0t6LtSNL6ix2gkmeA',
         'YC CDN Support': 'TImH4R8P2s85ZGIy',
         'Облако в Аркадии': 'B3-v1kHCm4XDWAM6rBx5iA',
         'YC Compliance': 'D98VvhRohkGmig3iRM7FFQ',
         'YC Crypto Services Support': 'BtlnglR86TAxKriRDy55bA',
         'yc-cloud-functions': 'Bkq6nBSfUyhVhICt5xYnVA',
         'Monitoring Community (Monitoring, Solomon, Golovan/YASM, Graphite, Grafana)': 'AWy4SEK1sYQoq4Ign2oINQ',
         'YubiKey Club': 'BXoEWA1ipyafiNNcjH5Awg',
         'GoRe PROD Support (formerly resps)': 'TunREb2h4a0SRWN8',
         'Яндекс.Облако для Яндекса': 'BpUNFEq8SrO8Yc0ltgo54g',
         'YC Audit Trails Support': 'ZjpNJjvimmEyMTQ6',
         'YC.NBS': 'Di0DqVRNpvD0aruOvsXM0w',
         'YC.Microsoft': 'SLCA0bZ-0ryl3u2g',
         'YC CRM news&help': 'H6ifoRCPCumekKeiH2jiiQ',
         'YC Assembly Workshop Support': 'DwJui1iKkkYti_GzMnV1Pw',
         '[NDA] Mobile App for Yandex.Cloud': 'EI31tVb7jhuPPIb0vlXtkQ',
         'YC DNS': 'D_mxLlB0C61MlBVpVkLeuw',
         'cloud-java': 'FCiHylHaMeqwFkxp60aKKg',
         'yc - Public CLI': 'Vf2UBILI5Bk1DIjl',
         'Data Transfer/Transfer Manager Support/DTSUPPORT': 'AqxpCBURpBhW79Uwq6Pcjw',
         'YC GIS (G-Cloud)': 'SslJw5VucKvK8MVZ',
         'hw-gov-lab': 'zI7Sccm_hkAxYjYy',
         'Cloud BLUE/GREEN/GOSCERT Support': 'e_KdqInMY8xmMThi',
         'CiC Swat Team': 'Xu2SBv1V4dY1MTli',
         'cloud-go': 'DsESjEl6yrfm3ZPgy9nhXw',
         'YC site announcements': 'AAAAAEcFWqlPw_TfsCPUow',
         'Cloud Incidents': '5Jv0nu39yZZiNWEy',
         'Yandex.Cloud Internal News': 'AAAAAFB9HKB5asglduYPew',
         'YCloud Feedback': 'AAAAAE_IM-KowxUBoIP7VA',
         'YC Prod Releases': 'AAAAAEgpR79LdEDRIYvM7g',
         'VPC Releases (YaIncBot)': 'V48tnHWBqfRTUn2U',
         'YC IAM & RM & Organizations Support': 'VUB51HMM76cIX2wD',
         'Audit Trails (ex. Cloud Trail)': 'RnEEAjTrxF00MTg6',
         'cgw issues work-group': 'G5RefBdOg-MZwyNVs-K2NQ',
         'YC Certificate Manager Support': 'OvyUuQQuvg8wZTMy',
         'Cloud_Biz_&_Support_Group': 'Uw-8Ymnw8NMqNjwN'}

''' Support + DataSphere': 'AXmZSlB9oTHDDdOKvXtUtg',
    'cloud-java-team': 'D_mxLg7sEEOz9u4DjSja_Q'
    'Yandex Database Chat': 'yandexdatabase_ru '''


api_id = 13574058
api_hash = "88aaa64e89db4258d71080d93bdf1846"
home = str(Path.home())
packagelocation = path.dirname(path.realpath(__file__))
context = ssl.create_default_context()
context.load_verify_locations(cafile=certifi.where())
client = httpx.AsyncClient(verify=context)


async def get_telegram_session():
    def get():
        try:
            with open(f"{home}/.telegramsession", "r") as s:
                session = s.read().strip()
                return StringSession(session)
        except:
            return StringSession(None)

    return await asyncio.to_thread(get)


async def save_telegram_session(session):
    def save(session):
        with open(f"{home}/.telegramsession", "w") as s:
            session = s.write(session)

    await asyncio.to_thread(save, session)


async def add_to_chat(client: TelegramClient, tqdm: Type[tqdm_asyncio], pbar: tqdm_asyncio, i: Tuple[str, str]):
    try:
        await client(tgfunctions.messages.ImportChatInviteRequest(i[1]))
    except tgerrors.rpcerrorlist.InviteHashExpiredError:
        tqdm.write(f"Invite link expired for chat {i[0]}")
    except (tgerrors.rpcerrorlist.InviteHashInvalidError, tgerrors.rpcerrorlist.InviteHashEmptyError):
        tqdm.write(f"Invalid link for chat {i[0]}")
    except tgerrors.rpcerrorlist.FloodWaitError as e:
        throttled = int(str(e).split(" ")[3])
        tqdm.write(f"Telegram api throttled for {throttled} seconds")
        for t in range(throttled):
            pbar.set_postfix(chat=i[0], sleep=f"{t}/{throttled} seconds")
            await asyncio.sleep(1)
        await add_to_chat(client, tqdm, pbar, i)
    except Exception as e:
        tqdm.write(f"While adding to chat {i[0]}, error: {e}")
    for t in range(10):
        pbar.set_postfix(chat=i[0], sleep=f"{t}/{10} seconds")
        await asyncio.sleep(1)


async def main():
    print(f"Recieved {len(chats)} chats")
    print("initializing telethon from $HOME/.telegramsession")
    session = await get_telegram_session()
    client = TelegramClient(retry_delay=3, request_retries=10, session=session, api_id=api_id, api_hash=api_hash)
    await client.start(code_callback=partial(asyncio.to_thread, input, "Please enter telegram code: "))
    await save_telegram_session(client.session.save())
    print("Adding you to chats")
    with tqdm(chats.items()) as pbar:
        async for i in pbar:
            i: Tuple[str, str]
            pbar.set_postfix(chat=i[0])
            await add_to_chat(client, tqdm, pbar, i)
    await client.disconnect()

asyncio.run(main())
