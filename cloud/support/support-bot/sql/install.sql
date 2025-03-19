-- ***************** SUPPORTBOT DB ******************;
-- ***************************************************;
--
--
-- // CREATE DATABASE BY FIRST
--
-- CREATE DATABASE supportbot DEFAULT CHARACTER SET utf8;
-- DEFAULT COLLATE utf8_general_ci;
--
--
-- DISABLE CHECKS FOR RE-CREATING TABLES
SET FOREIGN_KEY_CHECKS=0;

-- ==================== TABLES ======================;
-- ==================================================;
--
--  ******************** users **********************;

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
    `telegram_id`      bigint NOT NULL,
    `telegram_login`   varchar(255) NOT NULL,
    `staff_login`      varchar(255) NULL,
    `is_allowed`       tinyint DEFAULT 0 NOT NULL,
    `is_support`       tinyint DEFAULT 0 NOT NULL,
    PRIMARY KEY (`telegram_id`),
    INDEX (`staff_login`),
    UNIQUE (`telegram_id`, `staff_login`)
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  **************** users_config ******************;

DROP TABLE IF EXISTS `users_config`;
CREATE TABLE `users_config` (
    `telegram_id`             bigint NOT NULL,
    `is_admin`                tinyint DEFAULT 0 NOT NULL,
    `is_duty`                 tinyint DEFAULT 0 NOT NULL,
    `cloudabuse`              tinyint DEFAULT 0 NOT NULL,
    `cloudsupport`            tinyint DEFAULT 0 NOT NULL,
    `premium_support`         tinyint DEFAULT 0 NOT NULL,
    `business_support`        tinyint DEFAULT 0 NOT NULL,
    `standard_support`        tinyint DEFAULT 0 NOT NULL,
    `ycloud`                  tinyint DEFAULT 0 NOT NULL,
    `cloudops`                tinyint DEFAULT 0 NOT NULL,
    `work_time`               varchar(20) DEFAULT '10-19' NOT NULL,
    `notify_interval`         int DEFAULT 10 NOT NULL,
    `do_not_disturb`          tinyint DEFAULT 0 NOT NULL,
    `ignore_work_time`        tinyint DEFAULT 0 NOT NULL,
    PRIMARY KEY (`telegram_id`),
    UNIQUE (`telegram_id`),
    FOREIGN KEY (`telegram_id`)
      REFERENCES `users` (`telegram_id`)
      ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


-- ENABLE CHECKS FOR FK
SET FOREIGN_KEY_CHECKS=1;
