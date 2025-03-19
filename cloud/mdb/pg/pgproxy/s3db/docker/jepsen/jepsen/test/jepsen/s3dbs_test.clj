(ns jepsen.s3dbs_test
  (:require [clojure.test :refer :all]
            [jepsen.core :as jepsen]
            [jepsen.s3dbs :as s3dbs]))

(def pg_s3db_nodes ["s3db_s3db01_1.s3db_net"
                    "s3db_s3db02_1.s3db_net"])
(def pg_s3meta_nodes ["s3db_s3meta01_1.s3db_net"])
(def pg_s3proxy_nodes ["s3db_s3proxy_1.s3db_net"])
(def pg_pgmeta_nodes ["s3db_pgmeta_1.s3db_net"])

(deftest s3dbs-test
  (is (:valid? (:results (jepsen/run! (s3dbs/s3dbs-test pg_s3db_nodes pg_s3meta_nodes pg_s3proxy_nodes pg_pgmeta_nodes))))))
