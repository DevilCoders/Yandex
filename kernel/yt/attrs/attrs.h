#pragma once

/// @file attrs.h - Constants for YT meta-attributes

namespace NJupiter {
    /** Container struct for attribute names. For general attributes information see https://wiki.yandex-team.ru/yt/userdoc/tables/#atributy */
    struct TYtAttrName {
        static constexpr char const Sorted[]   = "sorted";
        static constexpr char const SortedBy[] = "sorted_by";

        static constexpr char const Schema[]     = "schema";
        static constexpr char const ReadSchema[] = "_read_schema";

        static constexpr char const SchemaMode[] = "schema_mode";

        enum ESchemaMode {
            SM_WEAK     /* "weak" */,
            SM_STRONG   /* "strong" */
        };

        static constexpr char const Type[] = "type";
        static constexpr char const Key[] = "key";

        static constexpr char const RowCount[]          = "row_count";
        static constexpr char const CompressionRatio[]  = "compression_ratio";
        static constexpr char const ReplicationFactor[] = "replication_factor";

        static constexpr char const ChunkCount[]           = "chunk_count";
        static constexpr char const CompressedDataSize[]   = "compressed_data_size";
        static constexpr char const UncompressedDataSize[] = "uncompressed_data_size";
        static constexpr char const DiskSpace[] = "resource_usage/disk_space";
        static constexpr char const ResourceUsage[] = "resource_usage";

        static constexpr char const CreationTime[] = "creation_time";
        static constexpr char const ModificationTime[] = "modification_time";
        static constexpr char const ExpirationTime[] = "expiration_time";

        static constexpr char const Revision[] = "revision";

        static constexpr char const CompressionCodec[]             = "compression_codec";
        static constexpr char const CompressionStatistics[]        = "compression_statistics";
        static constexpr char const ErasureCodec[]                 = "erasure_codec";
        static constexpr char const ErasureStatistics[]            = "erasure_statistics";
        static constexpr char const IntermediateCompressionCodec[] = "intermediate_compression_codec";

        static constexpr char const ExternalCellBias[] = "external_cell_bias";

        static constexpr char const ExternalMaster[] = "external";
        static constexpr char const Dynamic[] = "dynamic";

        static constexpr char const OptimizeFor[] = "optimize_for";

        static constexpr char const UserAttributeKeys[] = "user_attribute_keys";

        enum EOptimizeFor {
            OF_SCAN     /* "scan" */,
            OF_LOOKUP   /* "lookup" */
        };

        static constexpr char const PrimaryMedium[] = "primary_medium";

        enum EPrimaryMedium {
            PM_DEFAULT   /* "default" */,
            PM_SSD_BLOBS /* "ssd_blobs" */
        };

        static constexpr char const MaxRowWeight[] = "max_row_weight";
        static inline constexpr char const SuppressNightlyMerge[] = "suppress_nightly_merge";
        static constexpr char const DataWeight[] = "data_weight";

        static constexpr char const Account[] = "account";
        static constexpr char const Opaque[] = "opaque";


        // dynamic table attributes
        static constexpr char const MaxDataTTL[] = "max_data_ttl";
        static constexpr char const MinDataTTL[] = "min_data_ttl";
        static constexpr char const MaxDataVersions[] = "max_data_versions";
        static constexpr char const MinDataVersions[] = "min_data_versions";
        static constexpr char const AutoCompactionPeriod[] = "auto_compaction_period";
        static constexpr char const DynStoreAutoFlushPeriod[] = "dynamic_store_auto_flush_period";
        static constexpr char const EnableDynamicStoreRead[] = "enable_dynamic_store_read";
        static constexpr char const MergeRowsOnFlush[] = "merge_rows_on_flush";
        static constexpr char const EnableTabletBalancer[] = "enable_tablet_balancer";
        static constexpr char const DesiredTabletCount[] = "desired_tablet_count";
        static constexpr char const ChunkRowCount[] = "chunk_row_count";

        static constexpr char const EnableCompactionAndPartitioning[] = "enable_compaction_and_partitioning";

        static constexpr char const TabletBalancerConfig[] = "tablet_balancer_config";
        static constexpr char const EnableAutoReshard[] = "enable_auto_reshard";
        static constexpr char const EnableAutoTabletMove[] = "enable_auto_tablet_move";
        static constexpr char const TabletCount[] = "tablet_count";

        static constexpr char const TabletState[] = "tablet_state";
    };

    struct TYtUserAttrName {
        static constexpr char const BackupAttrPath[] = "backup";
    };

};
