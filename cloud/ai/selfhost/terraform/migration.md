# How to migrate old target

0. Get latest terraform release (`tfenv` could be used for version management)

1. Create new target folder under **targets/<target_group>**
   If new **<target_group>** is required `tfinit.py` should be updated accordingly

2. Create *common* subdirectory with corresponding layout:
   - main.tf :: main terraform file, contains *service_component* and *standard_service*
   - components.tf :: instantiated list of reusable components (e.g. solomon-agent)
   - variables.tf :: input variables for the common module, at least should contain
     * yandex_token
     * service_account_key_file
     * environment
     * folder_id
     * service_account_id
   - output.tf :: outputs of the module
   - secrets.tf

3. Instansiate components 

4. Provide description of the *service_component* according to *standard_service* input variable

5. Import *standard_service* module from modules and provide description of the service
   in required module fields

6. Create subfolders for each destination environment (preprod/staging/prod)
   and provide default layout for each of them
   - main.tf :: import common module here
   - state.tf :: specify terraform state storage
     (in S3 we use one storage so only key field should have different values)
   - variables.tf :: should contains two secrets as input variables
     * yandex_token
     * service_account_key_file

7. Init terraform in folder using `tfinit.py`

8. Import existing resources if you migrate terraform state into new key
   ```bash
   terraform import module.${module_name}.module.service.module.instance_group.ycp_microcosm_instance_group_instance_group.instance_group $resource_id
   ```

9. Plan with `terraform plan` and validate changes in diff
   Attention: new terraform versions print difference in infrastructure and state separately

10. Apply changes with `terraform apply`

11. ???

12. PROFIT