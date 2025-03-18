import asyncio
import os

from library.python.awssdk_async_extensions.lib import tvm2_session


async def main():
    tvm2_client_id = int(os.environ['TVM2_CLIENT_ID'])
    tvm2_client_secret = os.environ['TVM2_CLIENT_SECRET']
    tvm2_destination_id = 2000273
    access_key = os.environ.get('ACCESS_KEY')
    session = await tvm2_session(tvm2_client_id, tvm2_client_secret, tvm2_destination_id, access_key)
    client = session.client('s3', endpoint_url='http://s3.mds.yandex.net')
    for an_object in client.list_objects(Bucket='storage-admin')['Contents']:
        print(an_object)

if __name__ == '__main__':
    asyncio.run(main())
