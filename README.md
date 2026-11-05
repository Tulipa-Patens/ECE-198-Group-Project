# ECE-198-Group-Project
Waterloo ECE 1A Engineering Group Project


## Clone the Repository

At the terminal, go to the directory where you want to keep the local copy of the project.

Git will create a folder with the repository name and download the project files into it.

Copy the repository URL from the green **Code** button, then run:

```bash
git clone <repository-url>
cd <repository-name>
```

For example:

```bash
git clone https://github.com/username/project-name.git
cd project-name
```

You can check the repository connection with:

```bash
git remote -v
```

Because the repository is in the project owner's personal account, `origin` should point to the shared repository. All group members must have the necessary access permissions to push branches and contribute to the project.

## Open Your Personal Branch

Each group member should work on a separate feature branch. This prevents unfinished changes from being added directly to `main` and reduces the chance of interfering with other members' work.

First, make sure your local `main` branch is up to date:

```bash
git switch main
git pull --ff-only origin main
```

Create and switch to a new feature branch:

```bash
git switch -c <feature-branch-name>
```

Choose a descriptive branch name. For example:

```bash
git switch -c feature/login-page
git switch -c feature/database-setup
git switch -c bugfix/navigation-menu
git switch -c docs/setup-guide
```

A useful naming format is:

```text
feature/<short-description>
bugfix/<short-description>
docs/<short-description>
```

All changes and commits related to your task should be made on your feature branch, not directly on `main`.

You can check which branch you are currently using with:

```bash
git branch --show-current
```

## Commit and Push

Make your changes, then check the repository status:

```bash
git status
```

Stage the files you want to commit:

```bash
git add <file-name>
```

To stage all changed files, you can use:

```bash
git add .
```

However, review the files listed by `git status` before committing. Do not commit passwords, API keys, private information, generated files, or other files that should be excluded by `.gitignore`.

You can review the changes that are staged for the next commit with:

```bash
git diff --staged
```

Create a commit with a clear and descriptive message:

```bash
git commit -m "<commit-message>"
```

Conventional Commit messages are preferred but are not required. Common types include:

```text
feat:     add a new feature
fix:      correct a bug
docs:     update documentation
refactor: improve code without changing its behavior
test:     add or update tests
chore:    perform maintenance or configuration work
```

Push your feature branch to the shared repository:

```bash
git push -u origin <feature-branch-name>
```

The `-u` option connects your local branch to the remote branch. After the first push, you can usually push later changes by running:

```bash
git push
```

After pushing, create a pull request if your group uses pull requests for review. The pull request should merge your feature branch into the repository's `main` branch.

Before requesting a merge, run the project's required tests, formatting checks, and linting commands.

## Merge and Clean Up

Before merging, update your feature branch with the latest changes from `main`:

```bash
git switch <feature-branch-name>
git fetch origin
git merge origin/main
```

If Git reports conflicts, run:

```bash
git status
```

Open each conflicted file, resolve the conflicting sections, and remove the conflict markers:

```text
<<<<<<<
=======
>>>>>>>
```

After resolving the conflicts, stage the files and complete the merge:

```bash
git add <resolved-file-name>
git commit
```

Run the project's tests again after resolving conflicts.

If your group uses pull requests, push the updated branch:

```bash
git push
```

Then merge the pull request into `main after it has been reviewed and approved.

If group members are allowed to merge directly, use the following procedure after making sure that your feature branch is complete and tested:

```bash
git switch main
git pull --ff-only origin main
git merge <feature-branch-name>
git push origin main
```

The command:

```bash
git merge origin/main
```

does **not** merge your feature branch into `main`. It merges the remote `main` branch into whichever branch you are currently using. Therefore, run it while you are on your feature branch when you want to bring the latest `main` changes into that branch.

After your feature branch has been successfully merged and is no longer needed, update your local `main` branch:

```bash
git switch main
git pull --ff-only origin main
```

Delete the local feature branch:

```bash
git branch -d <feature-branch-name>
```

Delete the remote feature branch if it is no longer needed:

```bash
git push origin --delete <feature-branch-name>
```

Do not delete a branch if another group member is still using it or if it contains work that has not been merged.