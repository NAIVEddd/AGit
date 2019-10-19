# AGit
Git-clone impl.

## build
in 'build' folder, run `make`

## Usage
agit remote_repo local_path
#### example
agit git://127.0.0.1/AGit /agit

## daemon
git daemon --reuseaddr --verbose --base-path=(repo path) --export-all
