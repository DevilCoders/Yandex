# Terraform file organization

## Glossary
* **environment** - destination of deployement<br>
  - *preprod* - Cloud Preprod
  - *staging* - Cloud Prod that is not accessible by end users
  - *prod* - Cloud Prod
* **target** - set of terraform resources that should be deployed in different **environments** in the same manner 
* **module** - [terraform module](https://www.terraform.io/docs/language/modules/syntax.html), reusable piece of HCL
* **component** - specific type of **module** acting like abstraction for utility process that provides fixed output with:
  - *container* - description of Docker container that can be used during podmanifest generation
  - *configs* - map of rendered configs that should be deployed on target system

## Directories hierarchy
```
terraform
├── constants :: module with global shared constants that cannot be extracted from data sources
├── modules :: contatins all shared modules
│  ├── components :: contains all components (specific module type)
│  │  └── ...
│  └── ...
└── targets :: all targets combined in groups under directories
   ├── <some_group>
   │  ├── <some_datasphere_target>
   │  │  ├── common :: common configuration for target shared between all environments
   │  │  ├── preprod :: cloud preprod environment
   │  │  ├── staging :: internal cloud prod environment
   │  │  └── prod :: cloud prod enviornment
   │  └── ...
   └── ...
```