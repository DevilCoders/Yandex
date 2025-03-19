-- **************** CLOUDSUPPORT DB ******************;
-- ***************************************************;
--
--
-- // CREATE DATABASE BY FIRST
--
-- CREATE DATABASE cloudsupport DEFAULT CHARACTER SET utf8;
-- DEFAULT COLLATE utf8_general_ci;
--
--
-- DISABLE CHECKS FOR RE-CREATING TABLES
SET FOREIGN_KEY_CHECKS=0;

-- ================== TABLES ====================;
-- ==============================================;
--
--  ***************** issues ********************;

DROP TABLE IF EXISTS `issues`;
CREATE TABLE `issues` (
    `startrek_key`          varchar(255) NOT NULL,
    `startrek_id`           varchar(255) NOT NULL,
    `external_id`           varchar(255) NULL,
    `external_author`       varchar(255) NULL,
    `issue_author`          varchar(255) NULL,
    `type`                  varchar(50) NOT NULL,
    `status`                varchar(255) NOT NULL,
    `priority`              varchar(255) NOT NULL,
    `pay`                   varchar(45) NOT NULL,
    `cloud_id`              varchar(255) NULL,
    `billing_id`            varchar(255) NULL,
    `company_name`          varchar(255) NULL,
    `partner`               varchar(100) NULL,
    `account_manager`       varchar(255) NULL,
    `managed`               tinyint DEFAULT 0,
    `environment`           varchar(50) NULL,
    `issue_type_changed`    tinyint DEFAULT 0,
    `feedback_received`     tinyint DEFAULT 0,
    `created_at`            timestamp NOT NULL,
    `updated_at`            timestamp NOT NULL,
    `movedtoL2_at`          timestamp NOT NULL,
    `resolved_at`           timestamp DEFAULT NULL,
    PRIMARY KEY (`startrek_key`),
    INDEX (`account_manager`),
    UNIQUE (`startrek_key`, `startrek_id`)
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  **************** feedback ******************;

DROP TABLE IF EXISTS `feedback`;
CREATE TABLE `feedback` (
    `issue_key`               varchar(255) NOT NULL,
    `general`                 int DEFAULT 0 NOT NULL,
    `completely`              int DEFAULT 0 NOT NULL,
    `rapid`                   int DEFAULT 0 NOT NULL,
    `description`             text NULL,
    `created_at`              timestamp NOT NULL,
    PRIMARY KEY (`issue_key`),
    UNIQUE (`issue_key`),
    FOREIGN KEY (`issue_key`)
      REFERENCES `issues` (`startrek_key`)
      ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  **************** inbox ******************;

DROP TABLE IF EXISTS `inbox`;
CREATE TABLE `inbox` (
    `startrek_id`             varchar(255) NOT NULL,
    `issue_key`               varchar(255) NOT NULL,
    `author`                  varchar(255) NOT NULL,
    `created_at`              timestamp NOT NULL,
    PRIMARY KEY (`startrek_id`),
    UNIQUE (`startrek_id`),
    INDEX (`author`),
    FOREIGN KEY (`issue_key`)
      REFERENCES `issues` (`startrek_key`)
      ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  **************** comments ******************;

DROP TABLE IF EXISTS `comments`;
CREATE TABLE `comments` (
    `startrek_id`                   varchar(255) NOT NULL,
    `start_conversation_id`         varchar(255) NULL,
    `issue_key`                     varchar(255) NOT NULL,
    `start_conversation_index`      int DEFAULT 0,
    `comment_index`                 int DEFAULT 0,
    `author`                        varchar(255) NOT NULL,
    `sla`                           float DEFAULT 0.0,
    `raw_sla`                       float DEFAULT 0.0,
    `sla_as_str`                    varchar(100) NOT NULL,
    `sla_failed`                    tinyint DEFAULT 0,
    `dev_wait_time`                 float DEFAULT 0.0,
    `secondary_response`            tinyint DEFAULT 0,
    `reopened`                      tinyint DEFAULT 0,
    `mistake_comment_type`          tinyint DEFAULT 0,
    `issue_type_changed`            tinyint DEFAULT 0,
    `link`                          varchar(255) NULL,
    `start_conversation_link`       varchar(255) NULL,
    `created_at`                    timestamp NULL,
    PRIMARY KEY (`startrek_id`),
    UNIQUE (`startrek_id`),
    INDEX (`author`),
    FOREIGN KEY (`issue_key`)
      REFERENCES `issues` (`startrek_key`)
      ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  ***************** components *******************;

DROP TABLE IF EXISTS `components`;
CREATE TABLE `components` (
    `name`      varchar(255) NOT NULL,
    PRIMARY KEY (`name`),
    UNIQUE (`name`)
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  ******************* tags ***********************;

DROP TABLE IF EXISTS `tags`;
CREATE TABLE `tags` (
    `name`      varchar(255) NOT NULL,
    PRIMARY KEY (`name`),
    UNIQUE (`name`)
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  *************** issue_components ****************;

DROP TABLE IF EXISTS `issue_components`;
CREATE TABLE `issue_components` (
    `name`            varchar(255) NOT NULL,
    `startrek_key`    varchar(255) NOT NULL,
    PRIMARY KEY (`name`, `startrek_key`),
    FOREIGN KEY (`name`)
      REFERENCES `components` (`name`)
      ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (`startrek_key`)
      REFERENCES `issues` (`startrek_key`)
      ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


--  ******************* issue_tags *******************;

DROP TABLE IF EXISTS `issue_tags`;
CREATE TABLE `issue_tags` (
    `name`            varchar(255) NOT NULL,
    `startrek_key`    varchar(255) NOT NULL,
    PRIMARY KEY (`name`, `startrek_key`),
      FOREIGN KEY (`name`)
      REFERENCES `tags` (`name`)
      ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (`startrek_key`)
      REFERENCES `issues` (`startrek_key`)
      ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;


-- ENABLE CHECKS FOR FK
SET FOREIGN_KEY_CHECKS=1;
