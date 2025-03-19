# Changelog
  
## version 0.1.4 BETA
  
**Released 06-06-2020**
  
* New workers:  
    * InProgressNotifier: notifies about issues in progress at the end of the working day  
  
* New commands:  
    * /team – show team schedule  
  
* Changes:  
    * User:  
        * New attribute: is_support  
    * Database:  
        * Fix database init and  
    * Other:  
        * Fix issue title bug  
        * New column in the database (is_support)   
        * Performance optimization  
        * Format & other minor fixes  
  
## version 0.1.3 BETA
  
**Released 03-06-2020**
  
* New commands:  
    * Opened issues  
    * In progress issues  
    * Help  
  
* Other:
    * New buttons in menu  
    * Added dockerfile & deploy readme  
  
## version 0.1.2 BETA
  
**Released 02-06-2020**
  
* New services:  
    * RespsClient light version  
  
* New objects:  
    * YandexCloudDuty  
    * Resp (secondary for YandexCloudDuty)  

* New workers:  
    * SecurityWatchdog: worker for delete dismissed employees  
  
* Other:  
    * Added config example  
    * Added readme  
    * Added app scheme (diagram)  
    * New directory for extensions created: `ext`  
    * Now all client-modules from `services` use the authorization tokens specified in the configuration file. 
    It is still possible to pass a different value. For an endpoint, the following assignment order is used: 
    the value from the argument, the value from the configuration file, and the default value.  
  
* Changes:  
    * Bot:  
        * Workers are now divided into two types: daily and repeated  
    * StartrekNotifier:  
        * The RespsClient is now used to get current duty  

    * User:  
        * Redesigned `__init__`  
        * Now, when initializing a user object, an attribute request from the Staff API is only executed 
        if the user is not in the database or forced verification is enabled  
        * Added forced verification via the API by argument  
        * Redesigned `_checkout` method  
    * UserConfig:  
        * Now the default `is_admin` attribute is set to True if the user is specified as the app developer  
    * Database:  
        * Added connection retries  
    * Request:  
        * Fix for packing the response from API if it is of the list type  
    * Config:  
        * Added endpoints  
    
## version 0.1.1 ALPHA
  
**Released 31-05-2020**
  
* Changes:
    * Redesigned menu (core/components/buttons.py)  

## version 0.1.0 ALPHA
  
**Released 30-05-2020**
  
* Added Bot Core  
* Added Database interface  
* Added components for Bot (commands, buttons, keyboards)  
* Added user settings menu  
  
* Workers:  
    * StartrekNotifier  
  
* Services:  
    * StaffClient  
    * StartrekClient  
