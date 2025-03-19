# Cloud adder

## Description
- Add clouders to tg chats

## Installation from bucket
1. Choose your OS type in s3 bucket https://console.cloud.yandex.ru/folders/b1gec59ckq5q715itd60/storage/buckets/bins?key=cloud-adder%2F
2. Click on the s3 object for going to download menu
3. Click on the three dots in the upper right corner to download the object

### Hardcore build from arcadia
1. Mount arcadia https://docs.yandex-team.ru/devtools/intro/quick-start-guide#mount
2. Build binary in directory arcadia/cloud/support/tools/cloud-adder: `ya m`

## How to use
1. Check file `ls cloud-adder`

2. Add the execute permission to file `chmod +x cloud-adder`

3. Run file `./cloud-adder`, write your phone-number, press `Enter`

4. Copy your personal code, which you receive from Telegram

5. Pass your code and press `Enter`, adding will start. If you are already in the chat, the addition will move to the next chat. If Telegram turns on its throttling protection, the script will wait for the necessary time and continue adding.

6. After adding to all chats, go to Active Sessions in Telegram Settings and delete the Clouder user from whom the addition was performed

### MacOS Security System is angry:
1. Push `Cancel`
2. Go to System Preferences → Security & Privacy → General
3. Push `Allow Anyway`
4. Run file again
5. Push `Open`


## How to add additional chats
1. Add invite link in dictionary "chats" in `cloud-adder.py`. (Example: "Chat_name": "chat_id")
2. Push commits
3. Rebuild binary `ya m --target-platform=linux` for linux and `ya m --target-platform=darwin` for mac os in directory "arcadia/cloud/support/tools/cloud-adder"
4. Upload binary files in s3 bucket https://console.cloud.yandex.ru/folders/b1gec59ckq5q715itd60/storage/buckets/bins?key=cloud-adder%2F
