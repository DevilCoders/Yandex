## Nginx component
### Options

Set up your pillar
```yaml
data:
    nginx:
        logs:
            files_num: 10  # how many files to store
            max_file_size_mb: 500  # megabytes before file is rotated
```
