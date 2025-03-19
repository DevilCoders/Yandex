# Changelog

## version 1.0.8
* ALB support
* YDB support
* auth by key [oauth deprecated but still there]

## version 1.0.6

**Released 08-11-2020**

* GNP support
* Add human readably Cloud ID support

BUG fixes:
* Change request type in S3 method
* Fix NRD (non replicated disk) metric

## version 1.0.5

**Released 27-05-2020**

* New features:
    * Displaying grants in billing metadata
    * Retries for gRPC errors
    * More understandable messages about gRPC errors

## version 1.0.4

**Released 20-05-2020**

* UX improvements:
    * Added `billing_metadata` option to config file for show billing metadata by default
    * Added `--show-balance` option (overwrite value from config)

* Bug fixes:
    * Fixed error for argument `--limit`

## version 1.0.3

**Released 19-05-2020**

* New services:
    * IoT (gRPC)
    * VPC (gRPC)
    * LoadBalancer (gRPC)

* Other:
    * UX improvements
    * Added default values for all services
    * Updated aliases
    * Added warning messages

## version 1.0.2

**Released 17-05-2020**

* New services:
    * Monitoring (gRPC)

* New features:
    * Update quota metrics from yaml file

## version 1.0.1

**Released 16-05-2020**

* New features:
    * Interactive mode:
        * Autocomplete for services
        * Support multiplier
        * Show billing metadata
    * General:
        * Added validators for subject & services
        * Set quota metrics to zero/default for all/list of services
        * Multiply service quota metrics to multiplier as argument
        * Highlighting messages, errors, exhausted quotas

* Config:
    * Endpoints support in the config file (not required)
    * Autogenerate config file if config not exist
    * Custom path to config file supported by --config arg

* Working without config file:
    * --token arg (takes IAM & OAuth token)
    * --ssl arg takes /path/to/allCAs.pem

## version 1.0.0

**Released 13-05-2020**
Total code refactoring.
Versions below 1.x.x is deprecated and no longer supported.

* Supported API:
    * gRPC
    * REST

* Supported environments:
    * PROD
    * PREPROD

* Services:
    * Compute + VPC (REST)
    * Container Registry (gRPC)
    * Instance Group (gRPC)
    * Kubernetes (gRPC)
    * Managed Database (REST)
    * Object Storage (REST)
    * Resource Manager (REST)
    * Serverless Functions (gRPC)
    * Serverless Triggers (gRPC)
