# Building the manylinux ODBC Docker Images

This guide explains how to build and publish the manylinux Docker images used by the CI pipeline. There are two images, one for each ODBC driver manager: iODBC and unixODBC.

## Current versions

| Component      | Version               |
|----------------|-----------------------|
| Base image     | `manylinux2014_x86_64` |
| iODBC          | 3.52.16               |
| unixODBC       | 2.3.14                |

## Repository layout

Each image has its own folder containing its `Dockerfile`:

```
manylinux2014_x86_64_iODBC/      # Dockerfile for the iODBC image
manylinux2014_x86_64_unixODBC/   # Dockerfile for the unixODBC image
```

## Prerequisites

- Docker installed and running
- An account on the container registry you push to (e.g. Docker Hub)
- Permission to push to the target repository

Log in to the registry before pushing:

```bash
docker login
```

## Set up variables

Set these once in your shell so the commands below can be copied directly. Replace `myusername` with your registry username or organization.

```bash
export REGISTRY_USER=myusername
export TAG=$(date +%Y%m%d)   # e.g. 20261116
```

## Build, tag and push

The steps are the same for both images; only the folder and image name differ.

### iODBC image

```bash
cd manylinux2014_x86_64_iODBC

# Build
docker build -t manylinux2014_x86_64_iodbc:latest .

# Tag
docker tag manylinux2014_x86_64_iodbc:latest \
  "$REGISTRY_USER/manylinux2014_x86_64_iodbc:$TAG"

# Push
docker push "$REGISTRY_USER/manylinux2014_x86_64_iodbc:$TAG"

cd ..
```

### unixODBC image

```bash
cd manylinux2014_x86_64_unixODBC

# Build
docker build -t manylinux2014_x86_64_unixodbc:latest .

# Tag
docker tag manylinux2014_x86_64_unixodbc:latest \
  "$REGISTRY_USER/manylinux2014_x86_64_unixodbc:$TAG"

# Push
docker push "$REGISTRY_USER/manylinux2014_x86_64_unixodbc:$TAG"

cd ..
```

## Verify the image (optional)

Before pushing, you can check that the driver manager is installed:

```bash
# iODBC
docker run --rm manylinux2014_x86_64_iodbc:latest iodbc-config --version

# unixODBC
docker run --rm manylinux2014_x86_64_unixodbc:latest odbcinst -j
```

## Use the new image in CI

The images are referenced in `azure-pipelines.yml`. After pushing, update the image tags there to the new `$TAG` value so the pipeline picks up the new build, then commit the change.

## Updating versions

To upgrade the base image, iODBC or unixODBC:

1. Edit the version in the relevant `Dockerfile`.
2. Rebuild, tag and push as described above.
3. Update the tag in `azure-pipelines.yml`.
4. Update the **Current versions** table at the top of this file.