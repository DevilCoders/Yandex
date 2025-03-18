## Install this libraries before run script
```
pip install azure-storage-blob  # if you will use Azure Blob Storage
pip install aioboto3  # if you will use S3
pip install keyring
pip install toloka-kit==0.1.20
pip install crowd-kit==0.0.7
pip install async_lru
pip install aiohttp
```


## Base configuration
Via ```toloka.ini``` file. Sush as:
- tokens
- pools id
- s3 connection


Some detailed configuration in code:
- rename fields
- new fields
- change labels
- new aggregation methods
- etc.

## Running pipline
```
>>> python pedestrian_pipline.py
```

## Logs
At ```logs``` folder. Each pool in different folder.
- ```info.log``` - for simple running operation
- ```error.log``` - for some real problems in pipline

Current level INFO. You can change it in ```handlers/base_handler.py:70```

## Store files on Azure Blob Storage
You can configure it in ```toloka.ini`` file:
- Secret.account_key
- AzureBlobStorage.account_name
- AzureBlobStorage.container_name - existing container
- AzureBlobStorage.sas_url_valid_days - SAS URL expire after days

Also you can check you Azure settings by simple call:
```
python file_store_abs.py
```
It uploads this Readme on Azure Blob Storage and show the link to you. You can visit this link and download this file. So it works.
Values for this test setted in py-file, not in ini-file.


## Store files on S3
This script could use S3 for storing files.
You can configure it in ```toloka.ini`` file:
- Secrets.s3_key_id
- Secrets.s3_access_key
- S3.bucket_name - existing bucket
- S3.url - path to cloud with S3


Also you can check you S3 settings by simple call:
```
python file_store_s3.py
```
It uploads this Readme on S3 and show the link to you. You can visit this link and download this file. So it works.


**No delete politics**

Right now script don't delete anything from S3. It's easier to anderstand why some image was accepted or rejected, when you have this image in the same url and can look at them.

If you want to delete old images from S3, you can do it manualy. Script stores images in folder ```tmp``` in folders with current date names (for excample ```2021-10-18```). So you can drop old folders: good images already stored in other folders.


## Secrets
To create new secrets just run
```
python add_password.py
```
Specify **name** for new secret and value. You must put it twice.

Then in ```toloka.ini``` in ```Secret``` section, set this **name** as setting value.

> All secrets stores on local machine, if you will move script to another machine, please don't forget about secrets.

## Results
All pipeline results stores as pandas files in the current folder (and it means that this information was checked):
- no_org.tsv - organizations, that does not exist
- main_tasks.tsv - existing organizations (good necessary photos)
- bonus_tasks.tsv - existing organisations (bonus photos about this organizaions)

Current script version doesn't process other bonus organizations.

All good files after checking via pipline will be stored on S3 in folders ```main``` and ```main_bonus```. Links to that file stored in tsv-files.

### To understand results
Script writes two files in ```logs``` folder.
1. ```decisions.tsv``` - file with all decisions: create new tasks, god/bad photo, reject/accept assignment, pay bonuses, etc.
2. ```answers.tsv``` -  file with all answers from all pools. Helps to understand why some image was rejected or accepted.

Both files could be read by pandas:
```python
import pandas
decisions = pandas.read_csv('decisions.tsv', sep='\t')
answers = pandas.read_csv('answers.tsv', sep='\t')

# all decisions by pedestrian_assignment_id
print(decisions[decisions['pedestrian_assignment_id']=='0001b28630--617074420a57cd7869ea73c0'])
# or all decisions by address
print(decisions[decisions['address']=='Сходненская ул., 56'])

# all answers by pedestrian_assignment_id or address
print(answers[answers['pedestrian_assignment_id']=='0001b28630--617074420a57cd7869ea73c0'])
print(answers[answers['address']=='Сходненская ул., 56'])

# but more useful if you else filter by pool type
print(answers[(answers['pedestrian_assignment_id']=='0001b28630--617074420a57cd7869ea73c0')&(answers['handler']=='relevance')])
```

## Re-run script
You can stop script (or if it stoped for some reason), and run it again. It don't process any responses again. It could be possible with folder ```saves```, where script stores all information and reeds it when you run script.

- if you want to move script to another computer/folder, please copy ```saves``` folder
- if you want to process new pool, please copy all script, check ```toloka.ini``` and delete ```saves``` folder.


## About how this script works
1. It consists of several observers. Now only four of them are interesting. One observer for each pool: Pedestrian, Visit, Quality, Relevance.
2. Each "Observer" includes its own "handler".
3. Pipeline made "steps" (once in a minute). On each step, they call each observer separately(let's say in parallel) to process their pool. Then check if we need to stop processing. And if all observers say "stop", the pipeline will stop. Another way it will take another step.
4. What does each observer do on each step. It gathers from Toloka all NEW answers from their pool. New answers - answers that we did not get in the previous step. And then the observer sends all these new answers to their handler.
5. Each handler tries to aggregate answers, make decisions and create tasks in the next pool (or accept/reject assignment).
6. So handler and observer have several variables in memory. After Each step all these variables write down on the disk to the "saves" folder. And will be read from the disk, when you run the pipeline.
7. Except for the pedestrian handler. It relies only on what tasks already exist in the visit validation pool. Because it creates new tasks in several subprocesses.


So. While the pipeline is running, it is trying to "move forward" tasks from one pool to another on each step. And according to that, while the pipeline is running and new assignments appear in the pedestrian pool, the pipeline immediately try to create tasks in the visit validation pool.


Now pipeline stops only if all pools will be closed. And after our hack, it stops never.

 
If you stop the pipeline and run it again, it starts working from the same place.
If you stop the pipeline, clear the "save" folder, and run it again, it could process some tasks twice.
I believe that the script must be running all the time, while your pools are opened. It's not enough to run it twice a day or something like that.