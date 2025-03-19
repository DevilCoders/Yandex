-- MySQL dump 10.13  Distrib 5.7.21-21, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database:
-- ------------------------------------------------------
-- Server version	5.7.21-21-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
/*!50717 SELECT COUNT(*) INTO @rocksdb_has_p_s_session_variables FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'performance_schema' AND TABLE_NAME = 'session_variables' */;
/*!50717 SET @rocksdb_get_is_supported = IF (@rocksdb_has_p_s_session_variables, 'SELECT COUNT(*) INTO @rocksdb_is_supported FROM performance_schema.session_variables WHERE VARIABLE_NAME=\'rocksdb_bulk_load\'', 'SELECT 0') */;
/*!50717 PREPARE s FROM @rocksdb_get_is_supported */;
/*!50717 EXECUTE s */;
/*!50717 DEALLOCATE PREPARE s */;
/*!50717 SET @rocksdb_enable_bulk_load = IF (@rocksdb_is_supported, 'SET SESSION rocksdb_bulk_load = 1', 'SET @rocksdb_dummy_bulk_load = 0') */;
/*!50717 PREPARE s FROM @rocksdb_enable_bulk_load */;
/*!50717 EXECUTE s */;
/*!50717 DEALLOCATE PREPARE s */;

--
-- Current Database: `cache`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `cache` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `cache`;

--
-- Table structure for table `cache_profile_friend`
--

DROP TABLE IF EXISTS `cache_profile_friend`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `cache_profile_friend` (
  `id` char(100) NOT NULL,
  `id2` int(5) NOT NULL,
  `format` enum('html','php') NOT NULL DEFAULT 'html',
  `make_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`,`make_date`,`id2`,`format`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `cache_profile_friend`
  ADD UNIQUE KEY `cache_uniq` (`id`,`format`),
  ADD KEY `make_date` (`make_date`),
  ADD KEY `id2` (`id2`);
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = cp1251 */ ;
/*!50003 SET character_set_results = cp1251 */ ;
/*!50003 SET collation_connection  = cp1251_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = '' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 TRIGGER cache_profile_friend_delete
BEFORE DELETE ON cache_profile_friend
FOR EACH ROW BEGIN
        SET @mm3= memc_set(concat('profile_friend:',OLD.id),'',1);
        SET @mm4= memc_set(concat('profile_friend:d:',OLD.id), '',1);


END */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `hs_actor_likes`
--

DROP TABLE IF EXISTS `hs_actor_likes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_actor_likes` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_actor_likes`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_afisha`
--

DROP TABLE IF EXISTS `hs_afisha`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_afisha` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_afisha`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_afisha_actor`
--

DROP TABLE IF EXISTS `hs_afisha_actor`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_afisha_actor` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_afisha_actor`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_WebApi`
--

DROP TABLE IF EXISTS `hs_api_WebApi`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_WebApi` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_WebApi`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_android`
--

DROP TABLE IF EXISTS `hs_api_android`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_android` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_android`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_beeline`
--

DROP TABLE IF EXISTS `hs_api_beeline`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_beeline` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_beeline`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_iphone`
--

DROP TABLE IF EXISTS `hs_api_iphone`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_iphone` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_iphone`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_kinobr`
--

DROP TABLE IF EXISTS `hs_api_kinobr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_kinobr` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_kinobr`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_win8`
--

DROP TABLE IF EXISTS `hs_api_win8`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_win8` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_win8`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_wph7`
--

DROP TABLE IF EXISTS `hs_api_wph7`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_wph7` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_wph7`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_api_wph8`
--

DROP TABLE IF EXISTS `hs_api_wph8`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_api_wph8` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_api_wph8`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_award`
--

DROP TABLE IF EXISTS `hs_award`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_award` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_award`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_best_box`
--

DROP TABLE IF EXISTS `hs_best_box`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_best_box` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_best_box`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_best_stat`
--

DROP TABLE IF EXISTS `hs_best_stat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_best_stat` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_best_stat`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_box_dvd`
--

DROP TABLE IF EXISTS `hs_box_dvd`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_box_dvd` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_box_dvd`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_boxm`
--

DROP TABLE IF EXISTS `hs_boxm`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_boxm` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_boxm`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_buy`
--

DROP TABLE IF EXISTS `hs_buy`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_buy` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_buy`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_buy_genre`
--

DROP TABLE IF EXISTS `hs_buy_genre`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_buy_genre` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_buy_genre`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_cannes_random_film`
--

DROP TABLE IF EXISTS `hs_cannes_random_film`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_cannes_random_film` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_cannes_random_film`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_cast`
--

DROP TABLE IF EXISTS `hs_cast`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_cast` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_cast`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_cf`
--

DROP TABLE IF EXISTS `hs_cf`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_cf` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_cf`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_chart_registrations`
--

DROP TABLE IF EXISTS `hs_chart_registrations`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_chart_registrations` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_chart_registrations`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_chart_review_rating`
--

DROP TABLE IF EXISTS `hs_chart_review_rating`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_chart_review_rating` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_chart_review_rating`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_could_like`
--

DROP TABLE IF EXISTS `hs_could_like`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_could_like` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_could_like`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_digital_release`
--

DROP TABLE IF EXISTS `hs_digital_release`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_digital_release` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_digital_release`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_events`
--

DROP TABLE IF EXISTS `hs_events`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_events` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_events`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_fav_comments`
--

DROP TABLE IF EXISTS `hs_fav_comments`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_fav_comments` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_fav_comments`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_favourite_friends`
--

DROP TABLE IF EXISTS `hs_favourite_friends`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_favourite_friends` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_favourite_friends`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_feature`
--

DROP TABLE IF EXISTS `hs_feature`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_feature` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_feature`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film`
--

DROP TABLE IF EXISTS `hs_film`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating`
--

DROP TABLE IF EXISTS `hs_film_rating`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_bkp`
--

DROP TABLE IF EXISTS `hs_film_rating_bkp`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_bkp` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_bkp`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_fav`
--

DROP TABLE IF EXISTS `hs_film_rating_fav`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_fav` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_fav`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_full`
--

DROP TABLE IF EXISTS `hs_film_rating_full`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_full` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_full`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_likes`
--

DROP TABLE IF EXISTS `hs_film_rating_likes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_likes` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_likes`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user0`
--

DROP TABLE IF EXISTS `hs_film_rating_user0`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user0` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user0`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user1`
--

DROP TABLE IF EXISTS `hs_film_rating_user1`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user1` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user1`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user2`
--

DROP TABLE IF EXISTS `hs_film_rating_user2`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user2` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user2`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user3`
--

DROP TABLE IF EXISTS `hs_film_rating_user3`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user3` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user3`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user4`
--

DROP TABLE IF EXISTS `hs_film_rating_user4`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user4` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user4`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user5`
--

DROP TABLE IF EXISTS `hs_film_rating_user5`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user5` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user5`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user6`
--

DROP TABLE IF EXISTS `hs_film_rating_user6`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user6` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user6`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user7`
--

DROP TABLE IF EXISTS `hs_film_rating_user7`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user7` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user7`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user8`
--

DROP TABLE IF EXISTS `hs_film_rating_user8`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user8` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user8`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_film_rating_user9`
--

DROP TABLE IF EXISTS `hs_film_rating_user9`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_film_rating_user9` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_film_rating_user9`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_flyer`
--

DROP TABLE IF EXISTS `hs_flyer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_flyer` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_flyer`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_folder`
--

DROP TABLE IF EXISTS `hs_folder`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_folder` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_folder`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_friends_await`
--

DROP TABLE IF EXISTS `hs_friends_await`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_friends_await` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_friends_await`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_graph`
--

DROP TABLE IF EXISTS `hs_graph`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_graph` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_graph`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_guess`
--

DROP TABLE IF EXISTS `hs_guess`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_guess` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_guess`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_hs_api_wph8`
--

DROP TABLE IF EXISTS `hs_hs_api_wph8`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_hs_api_wph8` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_hs_api_wph8`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_icheck`
--

DROP TABLE IF EXISTS `hs_icheck`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_icheck` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_icheck`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_instagram`
--

DROP TABLE IF EXISTS `hs_instagram`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_instagram` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_instagram`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr`
--

DROP TABLE IF EXISTS `hs_kadr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr_concept`
--

DROP TABLE IF EXISTS `hs_kadr_concept`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr_concept` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr_concept`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr_events`
--

DROP TABLE IF EXISTS `hs_kadr_events`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr_events` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr_events`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr_promo`
--

DROP TABLE IF EXISTS `hs_kadr_promo`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr_promo` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr_promo`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr_sess`
--

DROP TABLE IF EXISTS `hs_kadr_sess`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr_sess` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr_sess`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr_sp`
--

DROP TABLE IF EXISTS `hs_kadr_sp`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr_sp` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr_sp`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_kadr_still`
--

DROP TABLE IF EXISTS `hs_kadr_still`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_kadr_still` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_kadr_still`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_keyword`
--

DROP TABLE IF EXISTS `hs_keyword`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_keyword` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_keyword`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_main`
--

DROP TABLE IF EXISTS `hs_main`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_main` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_main`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_mob`
--

DROP TABLE IF EXISTS `hs_mob`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_mob` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_mob`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_mob_detector`
--

DROP TABLE IF EXISTS `hs_mob_detector`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_mob_detector` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_mob_detector`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_news_calendar`
--

DROP TABLE IF EXISTS `hs_news_calendar`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_news_calendar` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_news_calendar`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_opera_api`
--

DROP TABLE IF EXISTS `hs_opera_api`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_opera_api` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_opera_api`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_oscar_game`
--

DROP TABLE IF EXISTS `hs_oscar_game`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_oscar_game` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_oscar_game`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_people`
--

DROP TABLE IF EXISTS `hs_people`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_people` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_people`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_pictures`
--

DROP TABLE IF EXISTS `hs_pictures`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_pictures` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_pictures`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_podcast`
--

DROP TABLE IF EXISTS `hs_podcast`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_podcast` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_podcast`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_popup_info`
--

DROP TABLE IF EXISTS `hs_popup_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_popup_info` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_popup_info`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_poster`
--

DROP TABLE IF EXISTS `hs_poster`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_poster` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_poster`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_poster_cover`
--

DROP TABLE IF EXISTS `hs_poster_cover`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_poster_cover` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_poster_cover`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_poster_fanart`
--

DROP TABLE IF EXISTS `hs_poster_fanart`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_poster_fanart` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_poster_fanart`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_premier`
--

DROP TABLE IF EXISTS `hs_premier`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_premier` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_premier`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_premier_expected`
--

DROP TABLE IF EXISTS `hs_premier_expected`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_premier_expected` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_premier_expected`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_profile_vote`
--

DROP TABLE IF EXISTS `hs_profile_vote`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_profile_vote` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_profile_vote`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_random_film`
--

DROP TABLE IF EXISTS `hs_random_film`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_random_film` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_random_film`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_release_premier`
--

DROP TABLE IF EXISTS `hs_release_premier`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_release_premier` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_release_premier`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_review`
--

DROP TABLE IF EXISTS `hs_review`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_review` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_review`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_robot`
--

DROP TABLE IF EXISTS `hs_robot`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_robot` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_robot`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_sequel`
--

DROP TABLE IF EXISTS `hs_sequel`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_sequel` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_sequel`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_shab_compiled`
--

DROP TABLE IF EXISTS `hs_shab_compiled`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_shab_compiled` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_shab_compiled`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_shop_ban_prod`
--

DROP TABLE IF EXISTS `hs_shop_ban_prod`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_shop_ban_prod` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_shop_ban_prod`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_site`
--

DROP TABLE IF EXISTS `hs_site`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_site` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_site`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_soundtrack`
--

DROP TABLE IF EXISTS `hs_soundtrack`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_soundtrack` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_soundtrack`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_studio`
--

DROP TABLE IF EXISTS `hs_studio`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_studio` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_studio`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_top_banner`
--

DROP TABLE IF EXISTS `hs_top_banner`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_top_banner` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_top_banner`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_totalizator`
--

DROP TABLE IF EXISTS `hs_totalizator`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_totalizator` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_totalizator`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_trailer`
--

DROP TABLE IF EXISTS `hs_trailer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_trailer` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_trailer`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_trivia`
--

DROP TABLE IF EXISTS `hs_trivia`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_trivia` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_trivia`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_tv`
--

DROP TABLE IF EXISTS `hs_tv`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_tv` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_tv`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_tv_widget`
--

DROP TABLE IF EXISTS `hs_tv_widget`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_tv_widget` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_tv_widget`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_twitter`
--

DROP TABLE IF EXISTS `hs_twitter`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_twitter` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_twitter`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_venice_random_film`
--

DROP TABLE IF EXISTS `hs_venice_random_film`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_venice_random_film` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_venice_random_film`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_vod`
--

DROP TABLE IF EXISTS `hs_vod`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_vod` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_vod`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_votes`
--

DROP TABLE IF EXISTS `hs_votes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_votes` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_votes`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_walk_fame`
--

DROP TABLE IF EXISTS `hs_walk_fame`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_walk_fame` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_walk_fame`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_wallpaper`
--

DROP TABLE IF EXISTS `hs_wallpaper`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_wallpaper` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_wallpaper`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Table structure for table `hs_weekend`
--

DROP TABLE IF EXISTS `hs_weekend`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hs_weekend` (
  `id` varchar(255) NOT NULL,
  `idobj` varchar(50) NOT NULL,
  `data` longtext NOT NULL,
  `ttl` int(5) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=cp1251;
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `hs_weekend`
  ADD KEY `idobj` (`idobj`),
  ADD KEY `ttl` (`ttl`);

--
-- Current Database: `mysql`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mysql` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `mysql`;

--
-- Table structure for table `columns_priv`
--

DROP TABLE IF EXISTS `columns_priv`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `columns_priv` (
  `Host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Db` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `User` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Table_name` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Column_name` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `Column_priv` set('Select','Insert','Update','References') CHARACTER SET utf8 NOT NULL DEFAULT '',
  PRIMARY KEY (`Host`,`Db`,`User`,`Table_name`,`Column_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Column privileges';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `db`
--

DROP TABLE IF EXISTS `db`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `db` (
  `Host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Db` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `User` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Select_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Insert_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Update_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Delete_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Drop_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Grant_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `References_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Index_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Alter_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_tmp_table_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Lock_tables_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_view_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Show_view_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_routine_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Alter_routine_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Execute_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Event_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Trigger_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  PRIMARY KEY (`Host`,`Db`,`User`),
  KEY `User` (`User`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Database privileges';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `engine_cost`
--

DROP TABLE IF EXISTS `engine_cost`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `engine_cost` (
  `engine_name` varchar(64) NOT NULL,
  `device_type` int(11) NOT NULL,
  `cost_name` varchar(64) NOT NULL,
  `cost_value` float DEFAULT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `comment` varchar(1024) DEFAULT NULL,
  PRIMARY KEY (`cost_name`,`engine_name`,`device_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `event`
--

DROP TABLE IF EXISTS `event`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `event` (
  `db` char(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT '',
  `name` char(64) NOT NULL DEFAULT '',
  `body` longblob NOT NULL,
  `definer` char(93) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT '',
  `execute_at` datetime DEFAULT NULL,
  `interval_value` int(11) DEFAULT NULL,
  `interval_field` enum('YEAR','QUARTER','MONTH','DAY','HOUR','MINUTE','WEEK','SECOND','MICROSECOND','YEAR_MONTH','DAY_HOUR','DAY_MINUTE','DAY_SECOND','HOUR_MINUTE','HOUR_SECOND','MINUTE_SECOND','DAY_MICROSECOND','HOUR_MICROSECOND','MINUTE_MICROSECOND','SECOND_MICROSECOND') DEFAULT NULL,
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `modified` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `last_executed` datetime DEFAULT NULL,
  `starts` datetime DEFAULT NULL,
  `ends` datetime DEFAULT NULL,
  `status` enum('ENABLED','DISABLED','SLAVESIDE_DISABLED') NOT NULL DEFAULT 'ENABLED',
  `on_completion` enum('DROP','PRESERVE') NOT NULL DEFAULT 'DROP',
  `sql_mode` set('REAL_AS_FLOAT','PIPES_AS_CONCAT','ANSI_QUOTES','IGNORE_SPACE','NOT_USED','ONLY_FULL_GROUP_BY','NO_UNSIGNED_SUBTRACTION','NO_DIR_IN_CREATE','POSTGRESQL','ORACLE','MSSQL','DB2','MAXDB','NO_KEY_OPTIONS','NO_TABLE_OPTIONS','NO_FIELD_OPTIONS','MYSQL323','MYSQL40','ANSI','NO_AUTO_VALUE_ON_ZERO','NO_BACKSLASH_ESCAPES','STRICT_TRANS_TABLES','STRICT_ALL_TABLES','NO_ZERO_IN_DATE','NO_ZERO_DATE','INVALID_DATES','ERROR_FOR_DIVISION_BY_ZERO','TRADITIONAL','NO_AUTO_CREATE_USER','HIGH_NOT_PRECEDENCE','NO_ENGINE_SUBSTITUTION','PAD_CHAR_TO_FULL_LENGTH') NOT NULL DEFAULT '',
  `comment` char(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT '',
  `originator` int(10) unsigned NOT NULL,
  `time_zone` char(64) CHARACTER SET latin1 NOT NULL DEFAULT 'SYSTEM',
  `character_set_client` char(32) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `collation_connection` char(32) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `db_collation` char(32) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `body_utf8` longblob,
  PRIMARY KEY (`db`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Events';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `func`
--

DROP TABLE IF EXISTS `func`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `func` (
  `name` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `ret` tinyint(1) NOT NULL DEFAULT '0',
  `dl` char(128) COLLATE utf8_bin NOT NULL DEFAULT '',
  `type` enum('function','aggregate') CHARACTER SET utf8 NOT NULL,
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='User defined functions';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `help_category`
--

DROP TABLE IF EXISTS `help_category`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `help_category` (
  `help_category_id` smallint(5) unsigned NOT NULL,
  `name` char(64) NOT NULL,
  `parent_category_id` smallint(5) unsigned DEFAULT NULL,
  `url` text NOT NULL,
  PRIMARY KEY (`help_category_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='help categories';
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `help_category` ADD UNIQUE KEY `name` (`name`);

--
-- Table structure for table `help_keyword`
--

DROP TABLE IF EXISTS `help_keyword`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `help_keyword` (
  `help_keyword_id` int(10) unsigned NOT NULL,
  `name` char(64) NOT NULL,
  PRIMARY KEY (`help_keyword_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='help keywords';
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `help_keyword` ADD UNIQUE KEY `name` (`name`);

--
-- Table structure for table `help_relation`
--

DROP TABLE IF EXISTS `help_relation`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `help_relation` (
  `help_topic_id` int(10) unsigned NOT NULL,
  `help_keyword_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`help_keyword_id`,`help_topic_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='keyword-topic relation';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `help_topic`
--

DROP TABLE IF EXISTS `help_topic`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `help_topic` (
  `help_topic_id` int(10) unsigned NOT NULL,
  `name` char(64) NOT NULL,
  `help_category_id` smallint(5) unsigned NOT NULL,
  `description` text NOT NULL,
  `example` text NOT NULL,
  `url` text NOT NULL,
  PRIMARY KEY (`help_topic_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='help topics';
/*!40101 SET character_set_client = @saved_cs_client */;
ALTER TABLE `help_topic` ADD UNIQUE KEY `name` (`name`);

--
-- Table structure for table `innodb_index_stats`
--

DROP TABLE IF EXISTS `innodb_index_stats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `innodb_index_stats` (
  `database_name` varchar(64) COLLATE utf8_bin NOT NULL,
  `table_name` varchar(64) COLLATE utf8_bin NOT NULL,
  `index_name` varchar(64) COLLATE utf8_bin NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `stat_name` varchar(64) COLLATE utf8_bin NOT NULL,
  `stat_value` bigint(20) unsigned NOT NULL,
  `sample_size` bigint(20) unsigned DEFAULT NULL,
  `stat_description` varchar(1024) COLLATE utf8_bin NOT NULL,
  PRIMARY KEY (`database_name`,`table_name`,`index_name`,`stat_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin STATS_PERSISTENT=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `innodb_table_stats`
--

DROP TABLE IF EXISTS `innodb_table_stats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `innodb_table_stats` (
  `database_name` varchar(64) COLLATE utf8_bin NOT NULL,
  `table_name` varchar(64) COLLATE utf8_bin NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `n_rows` bigint(20) unsigned NOT NULL,
  `clustered_index_size` bigint(20) unsigned NOT NULL,
  `sum_of_other_index_sizes` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`database_name`,`table_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin STATS_PERSISTENT=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `ndb_binlog_index`
--

DROP TABLE IF EXISTS `ndb_binlog_index`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ndb_binlog_index` (
  `Position` bigint(20) unsigned NOT NULL,
  `File` varchar(255) NOT NULL,
  `epoch` bigint(20) unsigned NOT NULL,
  `inserts` int(10) unsigned NOT NULL,
  `updates` int(10) unsigned NOT NULL,
  `deletes` int(10) unsigned NOT NULL,
  `schemaops` int(10) unsigned NOT NULL,
  `orig_server_id` int(10) unsigned NOT NULL,
  `orig_epoch` bigint(20) unsigned NOT NULL,
  `gci` int(10) unsigned NOT NULL,
  `next_position` bigint(20) unsigned NOT NULL,
  `next_file` varchar(255) NOT NULL,
  PRIMARY KEY (`epoch`,`orig_server_id`,`orig_epoch`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `plugin`
--

DROP TABLE IF EXISTS `plugin`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `plugin` (
  `name` varchar(64) NOT NULL DEFAULT '',
  `dl` varchar(128) NOT NULL DEFAULT '',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='MySQL plugins';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `proc`
--

DROP TABLE IF EXISTS `proc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `proc` (
  `db` char(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT '',
  `name` char(64) NOT NULL DEFAULT '',
  `type` enum('FUNCTION','PROCEDURE') NOT NULL,
  `specific_name` char(64) NOT NULL DEFAULT '',
  `language` enum('SQL') NOT NULL DEFAULT 'SQL',
  `sql_data_access` enum('CONTAINS_SQL','NO_SQL','READS_SQL_DATA','MODIFIES_SQL_DATA') NOT NULL DEFAULT 'CONTAINS_SQL',
  `is_deterministic` enum('YES','NO') NOT NULL DEFAULT 'NO',
  `security_type` enum('INVOKER','DEFINER') NOT NULL DEFAULT 'DEFINER',
  `param_list` blob NOT NULL,
  `returns` longblob NOT NULL,
  `body` longblob NOT NULL,
  `definer` char(93) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT '',
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `modified` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `sql_mode` set('REAL_AS_FLOAT','PIPES_AS_CONCAT','ANSI_QUOTES','IGNORE_SPACE','NOT_USED','ONLY_FULL_GROUP_BY','NO_UNSIGNED_SUBTRACTION','NO_DIR_IN_CREATE','POSTGRESQL','ORACLE','MSSQL','DB2','MAXDB','NO_KEY_OPTIONS','NO_TABLE_OPTIONS','NO_FIELD_OPTIONS','MYSQL323','MYSQL40','ANSI','NO_AUTO_VALUE_ON_ZERO','NO_BACKSLASH_ESCAPES','STRICT_TRANS_TABLES','STRICT_ALL_TABLES','NO_ZERO_IN_DATE','NO_ZERO_DATE','INVALID_DATES','ERROR_FOR_DIVISION_BY_ZERO','TRADITIONAL','NO_AUTO_CREATE_USER','HIGH_NOT_PRECEDENCE','NO_ENGINE_SUBSTITUTION','PAD_CHAR_TO_FULL_LENGTH') NOT NULL DEFAULT '',
  `comment` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `character_set_client` char(32) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `collation_connection` char(32) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `db_collation` char(32) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `body_utf8` longblob,
  PRIMARY KEY (`db`,`name`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Stored Procedures';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `procs_priv`
--

DROP TABLE IF EXISTS `procs_priv`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `procs_priv` (
  `Host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Db` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `User` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Routine_name` char(64) CHARACTER SET utf8 NOT NULL DEFAULT '',
  `Routine_type` enum('FUNCTION','PROCEDURE') COLLATE utf8_bin NOT NULL,
  `Grantor` char(93) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Proc_priv` set('Execute','Alter Routine','Grant') CHARACTER SET utf8 NOT NULL DEFAULT '',
  `Timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`Host`,`Db`,`User`,`Routine_name`,`Routine_type`),
  KEY `Grantor` (`Grantor`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Procedure privileges';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `proxies_priv`
--

DROP TABLE IF EXISTS `proxies_priv`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `proxies_priv` (
  `Host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `User` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Proxied_host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Proxied_user` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `With_grant` tinyint(1) NOT NULL DEFAULT '0',
  `Grantor` char(93) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`Host`,`User`,`Proxied_host`,`Proxied_user`),
  KEY `Grantor` (`Grantor`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='User proxy privileges';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `server_cost`
--

DROP TABLE IF EXISTS `server_cost`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `server_cost` (
  `cost_name` varchar(64) NOT NULL,
  `cost_value` float DEFAULT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `comment` varchar(1024) DEFAULT NULL,
  PRIMARY KEY (`cost_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `servers`
--

DROP TABLE IF EXISTS `servers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `servers` (
  `Server_name` char(64) NOT NULL DEFAULT '',
  `Host` char(64) NOT NULL DEFAULT '',
  `Db` char(64) NOT NULL DEFAULT '',
  `Username` char(64) NOT NULL DEFAULT '',
  `Password` char(64) NOT NULL DEFAULT '',
  `Port` int(4) NOT NULL DEFAULT '0',
  `Socket` char(64) NOT NULL DEFAULT '',
  `Wrapper` char(64) NOT NULL DEFAULT '',
  `Owner` char(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`Server_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='MySQL Foreign Servers table';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `slave_master_info`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE IF NOT EXISTS `slave_master_info` (
  `Number_of_lines` int(10) unsigned NOT NULL COMMENT 'Number of lines in the file.',
  `Master_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL COMMENT 'The name of the master binary log currently being read from the master.',
  `Master_log_pos` bigint(20) unsigned NOT NULL COMMENT 'The master log position of the last read event.',
  `Host` char(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT '' COMMENT 'The host name of the master.',
  `User_name` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The user name used to connect to the master.',
  `User_password` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The password used to connect to the master.',
  `Port` int(10) unsigned NOT NULL COMMENT 'The network port used to connect to the master.',
  `Connect_retry` int(10) unsigned NOT NULL COMMENT 'The period (in seconds) that the slave will wait before trying to reconnect to the master.',
  `Enabled_ssl` tinyint(1) NOT NULL COMMENT 'Indicates whether the server supports SSL connections.',
  `Ssl_ca` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The file used for the Certificate Authority (CA) certificate.',
  `Ssl_capath` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The path to the Certificate Authority (CA) certificates.',
  `Ssl_cert` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The name of the SSL certificate file.',
  `Ssl_cipher` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The name of the cipher in use for the SSL connection.',
  `Ssl_key` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The name of the SSL key file.',
  `Ssl_verify_server_cert` tinyint(1) NOT NULL COMMENT 'Whether to verify the server certificate.',
  `Heartbeat` float NOT NULL,
  `Bind` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'Displays which interface is employed when connecting to the MySQL server',
  `Ignored_server_ids` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The number of server IDs to be ignored, followed by the actual server IDs',
  `Uuid` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The master server uuid.',
  `Retry_count` bigint(20) unsigned NOT NULL COMMENT 'Number of reconnect attempts, to the master, before giving up.',
  `Ssl_crl` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The file used for the Certificate Revocation List (CRL)',
  `Ssl_crlpath` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'The path used for Certificate Revocation List (CRL) files',
  `Enabled_auto_position` tinyint(1) NOT NULL COMMENT 'Indicates whether GTIDs will be used to retrieve events from the master.',
  `Channel_name` char(64) NOT NULL DEFAULT '' COMMENT 'The channel on which the slave is connected to a source. Used in Multisource Replication',
  `Tls_version` text CHARACTER SET utf8 COLLATE utf8_bin COMMENT 'Tls version',
  PRIMARY KEY (`Channel_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Master Information';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `slave_relay_log_info`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE IF NOT EXISTS `slave_relay_log_info` (
  `Number_of_lines` int(10) unsigned NOT NULL COMMENT 'Number of lines in the file or rows in the table. Used to version table definitions.',
  `Relay_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL COMMENT 'The name of the current relay log file.',
  `Relay_log_pos` bigint(20) unsigned NOT NULL COMMENT 'The relay log position of the last executed event.',
  `Master_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL COMMENT 'The name of the master binary log file from which the events in the relay log file were read.',
  `Master_log_pos` bigint(20) unsigned NOT NULL COMMENT 'The master log position of the last executed event.',
  `Sql_delay` int(11) NOT NULL COMMENT 'The number of seconds that the slave must lag behind the master.',
  `Number_of_workers` int(10) unsigned NOT NULL,
  `Id` int(10) unsigned NOT NULL COMMENT 'Internal Id that uniquely identifies this record.',
  `Channel_name` char(64) NOT NULL DEFAULT '' COMMENT 'The channel on which the slave is connected to a source. Used in Multisource Replication',
  PRIMARY KEY (`Channel_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Relay Log Information';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `slave_worker_info`
--

DROP TABLE IF EXISTS `slave_worker_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `slave_worker_info` (
  `Id` int(10) unsigned NOT NULL,
  `Relay_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `Relay_log_pos` bigint(20) unsigned NOT NULL,
  `Master_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `Master_log_pos` bigint(20) unsigned NOT NULL,
  `Checkpoint_relay_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `Checkpoint_relay_log_pos` bigint(20) unsigned NOT NULL,
  `Checkpoint_master_log_name` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `Checkpoint_master_log_pos` bigint(20) unsigned NOT NULL,
  `Checkpoint_seqno` int(10) unsigned NOT NULL,
  `Checkpoint_group_size` int(10) unsigned NOT NULL,
  `Checkpoint_group_bitmap` blob NOT NULL,
  `Channel_name` char(64) NOT NULL DEFAULT '' COMMENT 'The channel on which the slave is connected to a source. Used in Multisource Replication',
  PRIMARY KEY (`Channel_name`,`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Worker Information';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `tables_priv`
--

DROP TABLE IF EXISTS `tables_priv`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `tables_priv` (
  `Host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Db` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `User` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Table_name` char(64) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Grantor` char(93) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `Table_priv` set('Select','Insert','Update','Delete','Create','Drop','Grant','References','Index','Alter','Create View','Show view','Trigger') CHARACTER SET utf8 NOT NULL DEFAULT '',
  `Column_priv` set('Select','Insert','Update','References') CHARACTER SET utf8 NOT NULL DEFAULT '',
  PRIMARY KEY (`Host`,`Db`,`User`,`Table_name`),
  KEY `Grantor` (`Grantor`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Table privileges';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `time_zone`
--

DROP TABLE IF EXISTS `time_zone`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `time_zone` (
  `Time_zone_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `Use_leap_seconds` enum('Y','N') NOT NULL DEFAULT 'N',
  PRIMARY KEY (`Time_zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Time zones';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `time_zone_leap_second`
--

DROP TABLE IF EXISTS `time_zone_leap_second`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `time_zone_leap_second` (
  `Transition_time` bigint(20) NOT NULL,
  `Correction` int(11) NOT NULL,
  PRIMARY KEY (`Transition_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Leap seconds information for time zones';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `time_zone_name`
--

DROP TABLE IF EXISTS `time_zone_name`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `time_zone_name` (
  `Name` char(64) NOT NULL,
  `Time_zone_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Time zone names';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `time_zone_transition`
--

DROP TABLE IF EXISTS `time_zone_transition`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `time_zone_transition` (
  `Time_zone_id` int(10) unsigned NOT NULL,
  `Transition_time` bigint(20) NOT NULL,
  `Transition_type_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`Time_zone_id`,`Transition_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Time zone transitions';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `time_zone_transition_type`
--

DROP TABLE IF EXISTS `time_zone_transition_type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `time_zone_transition_type` (
  `Time_zone_id` int(10) unsigned NOT NULL,
  `Transition_type_id` int(10) unsigned NOT NULL,
  `Offset` int(11) NOT NULL DEFAULT '0',
  `Is_DST` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Abbreviation` char(8) NOT NULL DEFAULT '',
  PRIMARY KEY (`Time_zone_id`,`Transition_type_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 STATS_PERSISTENT=0 COMMENT='Time zone transition types';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `user`
--

DROP TABLE IF EXISTS `user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `user` (
  `Host` char(60) COLLATE utf8_bin NOT NULL DEFAULT '',
  `User` char(32) COLLATE utf8_bin NOT NULL DEFAULT '',
  `Select_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Insert_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Update_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Delete_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Drop_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Reload_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Shutdown_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Process_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `File_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Grant_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `References_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Index_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Alter_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Show_db_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Super_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_tmp_table_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Lock_tables_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Execute_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Repl_slave_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Repl_client_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_view_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Show_view_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_routine_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Alter_routine_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_user_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Event_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Trigger_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `Create_tablespace_priv` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `ssl_type` enum('','ANY','X509','SPECIFIED') CHARACTER SET utf8 NOT NULL DEFAULT '',
  `ssl_cipher` blob NOT NULL,
  `x509_issuer` blob NOT NULL,
  `x509_subject` blob NOT NULL,
  `max_questions` int(11) unsigned NOT NULL DEFAULT '0',
  `max_updates` int(11) unsigned NOT NULL DEFAULT '0',
  `max_connections` int(11) unsigned NOT NULL DEFAULT '0',
  `max_user_connections` int(11) unsigned NOT NULL DEFAULT '0',
  `plugin` char(64) COLLATE utf8_bin NOT NULL DEFAULT 'mysql_native_password',
  `authentication_string` text COLLATE utf8_bin,
  `password_expired` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  `password_last_changed` timestamp NULL DEFAULT NULL,
  `password_lifetime` smallint(5) unsigned DEFAULT NULL,
  `account_locked` enum('N','Y') CHARACTER SET utf8 NOT NULL DEFAULT 'N',
  PRIMARY KEY (`Host`,`User`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Users and global privileges';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `general_log`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE IF NOT EXISTS `general_log` (
  `event_time` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  `user_host` mediumtext NOT NULL,
  `thread_id` bigint(21) unsigned NOT NULL,
  `server_id` int(10) unsigned NOT NULL,
  `command_type` varchar(64) NOT NULL,
  `argument` mediumblob NOT NULL
) ENGINE=CSV DEFAULT CHARSET=utf8 COMMENT='General log';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `slow_log`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE IF NOT EXISTS `slow_log` (
  `start_time` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  `user_host` mediumtext NOT NULL,
  `query_time` time(6) NOT NULL,
  `lock_time` time(6) NOT NULL,
  `rows_sent` int(11) NOT NULL,
  `rows_examined` int(11) NOT NULL,
  `db` varchar(512) NOT NULL,
  `last_insert_id` int(11) NOT NULL,
  `insert_id` int(11) NOT NULL,
  `server_id` int(10) unsigned NOT NULL,
  `sql_text` mediumblob NOT NULL,
  `thread_id` bigint(21) unsigned NOT NULL
) ENGINE=CSV DEFAULT CHARSET=utf8 COMMENT='Slow log';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Current Database: `test`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `test` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `test`;
/*!50112 SET @disable_bulk_load = IF (@is_rocksdb_supported, 'SET SESSION rocksdb_bulk_load = @old_rocksdb_bulk_load', 'SET @dummy_rocksdb_bulk_load = 0') */;
/*!50112 PREPARE s FROM @disable_bulk_load */;
/*!50112 EXECUTE s */;
/*!50112 DEALLOCATE PREPARE s */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2018-05-31 17:23:12
