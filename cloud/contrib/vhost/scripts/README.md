### sync_with_arc.py

Synchronize git changes on the current branch with arcadia by creating a new
branch, applying commits from the current git branch since the first common
base commit and then submitting them as a pull request. The synchronization is
one-way, meaning arcadia local changes are not expected and are not synchronized
with git. The script verifies that both repositories are consistent at their
base matching commits, and aborts synchronization if that is not the case.

Arcadia branch & pull request are created automatically, the branch name is
formed by appending prefix `bb-sync-` to the curent git branch name (that is,
unless `arc-sync-branch` is specified explicitly).

The script also automatically determines whether or not it should create a new
pull request. It does so by checking whether the sync branch already
existed prior to running the script, and whether there is already a pull request
open from it. This behavior can be overriden via `--force-create-pr`.

Usage: `./sync_with_arc.py <path-to-arcadia-repo> --arc-sync-branch <branch-name>`

Where:
- `<path-to-arcadia-repo>` - path to the arcadia repository mount point. If you
don't have one use [this](https://docs.yandex-team.ru/devtools/intro/quick-start-guide)
guide to learn how to set it up.
- `<branch-name>` - a unique name for the arcadia branch, which is going to
contain all the new changes and be used for the pull request. It is recommended
that you use the same name as the branch that has just been merged into master
for consistency.

Additonal options:
- `--git-base-branch` - Branch to use as merge base for the sync branch,
defaults to `master`
- `--arc-target-branch` - Target branch in arcadia, defaults to 'trunk'
- `--arc-sync-branch` - Branch to use for the new changes from git, the PR source.
Created automatically if doesn't exist.
- `--pr-description` - Description to use for the arcadia pull request (if not yet
created), otherwise opens up a text editor when creating the PR for the first time
- `--force-create-pr` - Create a new arcadia pull request even if sync branch already existed
- `--dry-run` - Run without creating new branches/commits/PRs in the arcadia repo
- `--pr-publish` - Automatically publish the PR for review upon creation
- `--pr-automerge` - Automatically merge the PR after CI checks & review

After running the script a new branch `<branch-name>` will be created,
which is going to contain all the not-yet-synchronized changes and a new
pull request into arcadia trunk will be opened (unless a different
`arc-target-branch` was specified). You will also get a chance to
enter a PR description in your text editor.

### FAQ:
Q: My arc branch contains changes that have previously been merged into the
branch by someone else. What do I do now?
A: This is OK. It happens in case there are older changes, which have not yet
made their way into arcadia trunk. You can either wait for them to be merged
or bring them in with your own pull request. Either way works.

Q: How would I add new changes to a previosuly created branch/PR?
A: The script is able to detect whether a branch/PR already exists and simply
force push your changes on top if needed. All you would have to do is rerun it
with the same arguments.
