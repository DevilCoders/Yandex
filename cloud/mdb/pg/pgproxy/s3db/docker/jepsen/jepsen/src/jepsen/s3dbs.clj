(ns jepsen.s3dbs
  "Tests for S3 metadata databases"
  (:require [clojure.tools.logging :refer :all]
            [clojure.core.reducers :as r]
            [clojure.set :as set]
            [clojure.string :as string]
            [jepsen [tests :as tests]
                    [os :as os]
                    [db :as db]
                    [client :as client]
                    [control :as control]
                    [nemesis :as nemesis]
                    [generator :as gen]
                    [checker :as checker]
                    [util :as util :refer [timeout]]
                    [net :as net]]
            [knossos [op :as op]]
            [clojure.java.jdbc :as j]))

(def register (atom 0))

(defn open-conn
  "Given a JDBC connection spec, opens a new connection unless one already
  exists. JDBC represents open connections as a map with a :connection key.
  Won't open if a connection is already open."
  [spec]
  (if (:connection spec)
    spec
    (j/add-connection spec (j/get-connection spec))))

(defn close-conn
  "Given a spec with JDBC connection, closes connection and returns the spec w/o connection."
  [spec]
  (when-let [conn (:connection spec)]
    (.close conn))
  {:classname   (:classname spec)
   :subprotocol (:subprotocol spec)
   :subname     (:subname spec)
   :user        (:user spec)
   :password    (:password spec)})

(defmacro with-conn
  "This macro takes that atom and binds a connection for the duration of
  its body, automatically reconnecting on any
  exception."
  [[conn-sym conn-atom] & body]
  `(let [~conn-sym (locking ~conn-atom
                     (swap! ~conn-atom open-conn))]
     (try
       ~@body
       (catch Throwable t#
         (locking ~conn-atom
           (swap! ~conn-atom (comp open-conn close-conn)))
         (throw t#)))))

(defn conn-spec
  "Return postgresql connection spec for given node name"
  [node]
  {:classname   "org.postgresql.Driver"
   :subprotocol "postgresql"
   :subname     (str "//" (name node) ":5432/s3db?prepareThreshold=0")
   :user        "postgres"})

(defn noop-client
  "Noop client"
  []
  (reify client/Client
    (setup! [_ test]
      (info "noop-client setup"))
    (invoke! [this test op]
      (assoc op :type :info, :error "noop"))
    (close! [_ test])
    (teardown! [_ test] (info "teardown"))
    client/Reusable
    (reusable? [_ test] true)))

(defn proxy-client
  "pgproxy client"
  [conn]
  (reify client/Client
    (setup! [_ test]
      (info "pg-proxy client setup"))
    (open! [_ test node]
      (let [conn (atom (conn-spec node))]
        (cond (string/includes? (name node) "proxy")
              (proxy-client conn)
              true
              (noop-client))))
    (invoke! [this test op]
      (try
          (timeout 5000 (assoc op :type :info, :error "timeout")
            (with-conn [c conn]
              (case (:f op)
                :read (assoc op :type :ok,
                                :value (->> (j/query c ["SELECT name::int FROM v1_code.list_objects('jepsen', '11111111-1111-1111-1111-111111111111', NULL, NULL, NULL, i_limit=> NULL);"]
                                                     {:row-fn :name}) (vec) (set)))
                :add (do (j/query c [(str "select v1_code.add_object ('jepsen','11111111-1111-1111-1111-111111111111', 'disabled',"(get op :value)"::text, "(rand-int 100500)",'33333333-3333-3333-3333-333333333333',1,1,'22222222-2222-2222-2222-222222222222', i_storage_class=>"(rand-int 3)")")])
                            (assoc op :type :ok))
                :check (assoc op  :type :ok, :value (->> (control/on-many (:pg_s3db_nodes test)
                                (control/exec*
                                      "rm -f /tmp/errors")
                                (control/exec
                                      :/pg/pgproxy/s3db/scripts/s3db/check_chunks_counters/check_chunks_counters.py :-d "dbname=s3db user=postgres" :--critical-errors-filepath "/tmp/errors")
                                (control/exec
                                      :/pg/pgproxy/s3db/scripts/s3db/check_chunks_counters/check_chunks_counters.py :-d "dbname=s3db user=postgres" :--storage-class 1 :--critical-errors-filepath "/tmp/errors")
                                (control/exec*
                                      "[ -f /tmp/errors ] && wc -l /tmp/errors | awk '{print $1}' || echo '0'")
                        ) vals (map #(Integer/parseInt %)) (reduce +)))
                :check-meta (assoc op  :type :ok, :value (->> (control/on-many (:pg_s3meta_nodes test)
                                (control/exec*
                                      "rm -f /tmp/errors")
                                (control/exec
                                      :/pg/pgproxy/s3db/scripts/s3meta/check_chunks_bounds/check_chunks_bounds.py :-d "dbname=s3meta user=postgres" :-p "host=pgmeta dbname=s3db user=postgres" :--critical-errors-filepath "/tmp/errors")
                                (control/exec*
                                      "[ -f /tmp/errors ] && wc -l /tmp/errors | awk '{print $1}' || echo '0'")
                        ) vals (map #(Integer/parseInt %)) (reduce +)))
               )))
        (catch Throwable t#
          (let [m# (.getMessage t#)]
            (assoc op :type :info, :error m#)))))
    (close! [_ test] (close-conn conn))
    (teardown! [_ test])
    client/Reusable
    (reusable? [_ test] true)))

(defn db
  "PostgreSQL database"
  []
  (reify db/DB
    (setup! [_ test node]
      (info (str (name node) " setup")))

    (teardown! [_ test node]
      (info (str (name node) " teardown")))))

(defn r [_ _] {:type :invoke, :f :read, :value nil})
(defn a [_ _] {:type :invoke, :f :add, :value (swap! register (fn [current-state] (+ current-state 1)))})
(defn c [_ _] {:type :invoke, :f :check, :value nil})
(defn cm [_ _] {:type :invoke, :f :check-meta, :value nil})

(defn counters-updater
  "Executes update chunks counters"
  []
  (reify nemesis/Nemesis
    (setup! [this test]
      this)

    (invoke! [this test op]
             (case (:f op)
               :update-counters-meta  (assoc op :value
                 (control/on (rand-nth (:pg_s3meta_nodes test))
                       (control/exec
                             :/pg/pgproxy/s3db/scripts/s3meta/update_chunks_counters/update_chunks_counters.py :-d "dbname=s3meta user=postgres" :-p "host=pgmeta dbname=s3db user=postgres")))
               :update-counters-queue  (assoc op :value
                  (control/on (rand-nth (:pg_s3db_nodes test))
                       (control/exec
                             :psql "user=postgres dbname=s3db" :-c "SELECT util.update_chunks_counters()")))
               :update-shards-stat  (assoc op :value
                  (control/on (rand-nth (:pg_s3meta_nodes test))
                       (control/exec
                             :psql "user=postgres dbname=s3meta" :-c "SELECT v1_impl.refresh_shard_stat()")))
    ))

    (teardown! [this test]
      (info (str "teardown updater")))
    nemesis/Reflection
    (fs [this] #{})))

(defn scripts-runner
  "Executes python scripts"
  []
  (reify nemesis/Nemesis
    (setup! [this test]
      this)

    (invoke! [this test op]
             (case (:f op)
               :move  (assoc op :value
                 (control/on (rand-nth (:pg_s3meta_nodes test))
                       (control/exec
                             :/pg/pgproxy/s3db/scripts/s3meta/chunk_mover/chunk_mover.py :-d "dbname=s3meta user=postgres" :-p "host=pgmeta dbname=s3db user=postgres" :-m 2 :-t 10 :--max-threads 3 :--allow-same-shard :-D "1 minute")))
               :split  (assoc op :value
                  (control/on (rand-nth (:pg_s3db_nodes test))
                       (control/exec
                             :/pg/pgproxy/s3db/scripts/s3db/chunk_splitter/chunk_splitter.py :-d "dbname=s3db user=postgres" :-p "host=pgmeta dbname=s3db user=postgres" :-t 100)))
    ))

    (teardown! [this test]
      (info (str "teardown scripts runner")))
    nemesis/Reflection
    (fs [this] #{})))

(defn reindexer
  "Executes REINDEX CONCURRENTLY s3.objects"
  []
  (reify nemesis/Nemesis
    (setup! [this test]
      this)

    (invoke! [this test op]
             (case (:f op)
               :reindex  (assoc op :value
                 (control/on (rand-nth (:pg_s3db_nodes test))
                       (control/exec
                             :psql "user=postgres dbname=s3db" :-c "REINDEX INDEX CONCURRENTLY s3.objects_156_pkey")))
    ))

    (teardown! [this test]
      (info (str "teardown reindexer")))
    nemesis/Reflection
    (fs [this] #{})))

(def objects-set
  "Given a set of :add operations followed by a final :read, verifies that
  every successfully added element is present in the read, and that the read
  contains only elements for which an add was attempted."
  (reify checker/Checker
    (check [this test history opts]
      (let [attempts (->> history
                          (r/filter op/invoke?)
                          (r/filter #(= :add (:f %)))
                          (r/map :value)
                          (into #{}))
            adds (->> history
                      (r/filter op/ok?)
                      (r/filter #(= :add (:f %)))
                      (r/map :value)
                      (into #{}))
            errors (->> history
                      (r/filter op/ok?)
                      (r/filter (or #(= :check (:f %)) #(= :check-meta (:f %))))
                      (r/map :value)
                      (reduce +))
            final-read (->> history
                          (r/filter op/ok?)
                          (r/filter #(= :read (:f %)))
                          (r/map :value)
                          (reduce (fn [_ x] x) nil))]
        (if-not final-read
          {:valid? false
           :error  "Set was never read"}

          (let [; The OK set is every read value which we tried to add
                ok          (set/intersection final-read attempts)

                ; Unexpected records are those we *never* attempted.
                unexpected  (set/difference final-read attempts)

                ; Lost records are those we definitely added but weren't read
                lost        (set/difference adds final-read)

                ; Recovered records are those where we didn't know if the add
                ; succeeded or not, but we found them in the final set.
                recovered   (set/difference ok adds)]

            {:valid?          (and (empty? lost) (empty? unexpected) (= errors 0))
             :counters-errors errors
             :ok              (util/integer-interval-set-str ok)
             :lost            (util/integer-interval-set-str lost)
             :unexpected      (util/integer-interval-set-str unexpected)
             :recovered       (util/integer-interval-set-str recovered)
             :ok-frac         (util/fraction (count ok) (count attempts))
             :unexpected-frac (util/fraction (count unexpected) (count attempts))
             :lost-frac       (util/fraction (count lost) (count attempts))
             :recovered-frac  (util/fraction (count recovered) (count attempts))}
         ))))))

(defn s3dbs-test
  [pg_s3db_nodes pg_s3meta_nodes pg_s3proxy_nodes pg_pgmeta_nodes]
  {:nodes     (concat pg_s3db_nodes pg_s3meta_nodes pg_s3proxy_nodes pg_pgmeta_nodes)
   :pg_s3db_nodes pg_s3db_nodes
   :pg_s3meta_nodes pg_s3meta_nodes
   :pg_s3proxy_nodes pg_s3proxy_nodes
   :pg_pgmeta_nodes pg_pgmeta_nodes
   :name      "s3dbs"
   :os        os/noop
   :db        (db)
   :ssh       {:private-key-path "/root/.ssh/id_rsa"}
   :net       net/iptables
   :client    (proxy-client nil)
   :nemesis   (nemesis/compose {#{:update-counters-meta} (counters-updater)
                                #{:update-counters-queue} (counters-updater)
                                #{:update-shards-stat} (counters-updater)
                                #{:reindex} (reindexer)
                                #{:move} (scripts-runner)
                                #{:split} (scripts-runner)})
   :generator (gen/phases
                (->> a
                     (gen/stagger 1/50)
                     (gen/nemesis
                       (gen/cycle [{:type :info, :f :reindex}
                                   (gen/sleep 5)
                                   {:type :info, :f :update-counters-meta}
                                   (gen/sleep 5)
                                   {:type :info, :f :update-counters-queue}
                                   (gen/sleep 5)
                                   {:type :info, :f :update-shards-stat}
                                   (gen/sleep 5)
                                   {:type :info, :f :move}
                                   (gen/sleep 5)
                                   {:type :info, :f :split}
                                   (gen/sleep 5)]))
                     (gen/time-limit 1800))
                (->> (gen/mix [r c cm])
                     (gen/stagger 1)
                     (gen/clients)
                     (gen/time-limit 60)))
   :checker   objects-set
   :remote    control/ssh})
