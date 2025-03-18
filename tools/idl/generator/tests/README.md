# Internal tag tests

These tests rely heavily on the exact match of generated files.
If you made a change in generation process and broke the tests, you can re-generate those files in three simple steps:
```
cd generated_files_updater
ya make
./generated_files_updater
```
Now just review the changes that were applied to generated files, make sure it's what you wanted, and commmit them!
