

## [2.1.0] - 2019-08-13
## Added
- Duty command now can show list of upcoming duties /duty <team-name> <n-days>
- Job that notify IM about empty .csv with duty schedule

## [2.0.1] - 2019-08-06
## Changed
- Fixed telegram-staff bug with capitalized login

## [2.0.0] - 2019-08-05
## Added
- Incidents notifications
- Custom chat flagues for incidents notifications
- yc_announcements incidents notifications
- a lot of other staff for incidents
- ...
- Profit!

## [1.7.2] - 2019-08-05
## Added
- Can use /comment with staff_login

## [1.7.1] - 2019-08-02
## Changed
- Hotfix

## [1.7.0] - 2019-07-31
## Added
- Command duty_ticket for actual CLOUDDUTY

## [1.6.2] - 2019-07-30
## Changed
- "Whois" is now working with MessageHandler
- Duty start job notify formatting fixed

## [1.6.1] - 2019-07-29
## Changed
- Hotfix for boto3

## [1.6.0] - 2019-07-29
## Added
- Business-support tickets notification
- Bot usage stats
- Dynamic update handler by rotation time from database
- Notifications at duty start with custom text

## Changed 
- Abuse notifications now is working for nightly supports
- Suptickets notifications handler reworked
- Abuse notifications handler reworked
- Reply live in S3 instead of hardcode
- Update command also update reply data from s3

## [1.5.1] - 2019-07-18
## Changed
- Fix formatting in user __str__ representation

## [1.5.0] - 2019-07-18
### Added
- Duty notify flag for users 
### Changed
- Adapt job_duty_notify to not send if it's 2+ duty on row
- Fix job_duty_notify to not send if duty_notify set to False

## [1.4.1] - 2019-07-17
### Added
- Custom dictionaries for components
- Custom rotation time for components
- Command whois for startrek rsolving
- Aliases for basic comands
- Base for further components customizaiton
- Changelog
### Changed
- Fix worktime check condition for job_abuse, job_notify
- Adapt command_duty logic for custom dictionaries and rotation time.
- Adapt job_notify logic for futher customizations
- Update_cache logging structure
- Add new commands to doc
