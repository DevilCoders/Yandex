(ns jepsen.pgsync-test
  (:require [clojure.test :refer :all]
            [jepsen.core :as jepsen]
            [jepsen.pgsync :as pgsync]))

(def pg_nodes ["pgsync_postgresql1_1.pgsync_pgsync_net"
               "pgsync_postgresql2_1.pgsync_pgsync_net"
               "pgsync_postgresql3_1.pgsync_pgsync_net"])

(def zk_nodes ["pgsync_zookeeper1_1.pgsync_pgsync_net"
               "pgsync_zookeeper2_1.pgsync_pgsync_net"
               "pgsync_zookeeper3_1.pgsync_pgsync_net"])

(deftest pgsync-test
  (is (:valid? (:results (jepsen/run! (pgsync/pgsync-test pg_nodes zk_nodes))))))
