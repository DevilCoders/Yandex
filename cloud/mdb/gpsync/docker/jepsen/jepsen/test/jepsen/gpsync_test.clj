(ns jepsen.gpsync-test
  (:require [clojure.test :refer :all]
            [jepsen.core :as jepsen]
            [jepsen.gpsync :as gpsync]))

(def pg_nodes ["gpsync_postgresql1_1.gpsync_gpsync_net"
               "gpsync_postgresql2_1.gpsync_gpsync_net"
               "gpsync_postgresql3_1.gpsync_gpsync_net"])

(def zk_nodes ["gpsync_zookeeper1_1.gpsync_gpsync_net"
               "gpsync_zookeeper2_1.gpsync_gpsync_net"
               "gpsync_zookeeper3_1.gpsync_gpsync_net"])

(deftest gpsync-test
  (is (:valid? (:results (jepsen/run! (gpsync/gpsync-test pg_nodes zk_nodes))))))
