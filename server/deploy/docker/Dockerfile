# XiguChat 后端镜像：编译环境与运行环境合一
#
# 镜像内容：
#   · 工具链  GCC 11 + CMake 3.31 + Ninja + vcpkg（依赖版本由 vcpkg.json 的
#     builtin-baseline 锁定）
#   · 依赖    vcpkg 第三方库已预编译到 /opt/xiguchat/vcpkg_installed
#   · 存储    MySQL 8（开放 X 协议 33060）+ Redis
#   · 脚本    取代码 / 建库 / 编译 / 启动，见 deploy/docker/scripts
#
# 依赖之所以预编译：vcpkg 需从 GitHub 拉取依赖源码编译，耗时约 1 小时，
# 且要求构建机能直连 GitHub。预编译后，运行镜像的机器不必再联网下载。
#
# 构建（构建上下文为 server 目录）：
#   docker build -f deploy/docker/Dockerfile -t xiguchat-server:amd64 .
FROM --platform=linux/amd64 ubuntu:22.04

ARG VCPKG_COMMIT=b8b8df2201ad8509b81a830fe0957bcb98e06c27
ARG CMAKE_VERSION=3.31.6
# 构建期代理。网络受限时传入宿主机代理地址即可，例如
#   --build-arg PROXY=http://host.docker.internal:7890
# 只作用于构建过程，不会进入最终镜像
ARG PROXY=""

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Asia/Shanghai \
    LANG=C.UTF-8 \
    VCPKG_ROOT=/opt/vcpkg \
    VCPKG_FORCE_SYSTEM_BINARIES=1 \
    CCACHE_DIR=/opt/xiguchat/ccache \
    PATH="/opt/cmake/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

# ---------- 1. 编译工具链 ----------
RUN printf 'Acquire::Retries "5";\nAcquire::http::Timeout "30";\nAcquire::https::Timeout "30";\n' \
      > /etc/apt/apt.conf.d/99-retries \
 && export http_proxy="${PROXY}" https_proxy="${PROXY}" \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential \
      ninja-build \
      pkg-config \
      ca-certificates \
      curl \
      gnupg \
      git \
      zip unzip tar xz-utils bzip2 file \
      procps net-tools iproute2 less tzdata locales \
      ccache \
      autoconf autoconf-archive automake libtool patch make \
 && locale-gen zh_CN.UTF-8 \
 && rm -rf /var/lib/apt/lists/*

# ---------- 2. CMake ----------
# Ubuntu 22.04 仓库的 CMake 是 3.22，不满足 CMakePresets.json 要求的 3.27，这里装 Kitware 官方二进制
RUN export http_proxy="${PROXY}" https_proxy="${PROXY}" \
 && curl -fsSL -o /tmp/cmake.tar.gz \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
 && mkdir -p /opt/cmake \
 && tar --strip-components=1 -xzf /tmp/cmake.tar.gz -C /opt/cmake \
 && rm /tmp/cmake.tar.gz

# ---------- 3. vcpkg ----------
# checkout 到 vcpkg.json 的 builtin-baseline 对应提交，保证依赖版本一致
RUN export http_proxy="${PROXY}" https_proxy="${PROXY}" \
 && git clone --filter=blob:none https://github.com/microsoft/vcpkg.git /opt/vcpkg \
 && git -C /opt/vcpkg checkout ${VCPKG_COMMIT} \
 && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

# ---------- 4. 预编译第三方依赖 ----------
# 本层只依赖 vcpkg.json，与项目源码无关：改代码重建镜像时命中缓存，不会重跑。
# overlay triplet 把依赖限制为 release 变体（项目以 Release 构建，Debug 用不到），
# 可省掉近一半编译时间。
ENV VCPKG_OVERLAY_TRIPLETS=/opt/xiguchat/triplets
COPY deploy/docker/triplets /opt/xiguchat/triplets
COPY vcpkg.json /opt/xiguchat/deps/vcpkg.json
RUN --mount=type=cache,target=/opt/vcpkg/downloads \
    --mount=type=cache,target=/opt/vcpkg/buildtrees \
    --mount=type=cache,target=/opt/xiguchat/bincache \
    export http_proxy="${PROXY}" https_proxy="${PROXY}" \
       VCPKG_DEFAULT_BINARY_CACHE=/opt/xiguchat/bincache \
 && cd /opt/xiguchat/deps \
 && /opt/vcpkg/vcpkg install \
        --x-install-root=/opt/xiguchat/vcpkg_installed \
        --triplet=x64-linux \
 && rm -rf /opt/vcpkg/packages

# ---------- 5. MySQL 8 ----------
# 项目用 X DevAPI 连接 MySQL（33060 端口），需要 X 插件。Ubuntu 仓库的 mysql-server
# 不含该插件，因此从 MySQL 官方 APT 源安装；官方包把 X 插件编进了 mysqld，无需单独加载。
# 安装后清空数据目录：首次启动由 init_db.sh 以空口令初始化，避免包初始化时的随机口令。
COPY deploy/docker/mysql.cnf /etc/mysql/conf.d/xigu.cnf
RUN printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d \
 && chmod +x /usr/sbin/policy-rc.d \
 && export http_proxy="${PROXY}" https_proxy="${PROXY}" \
 && curl -fsSL https://repo.mysql.com/RPM-GPG-KEY-mysql-2025 \
      | gpg --dearmor > /usr/share/keyrings/mysql.gpg \
 && echo "deb [signed-by=/usr/share/keyrings/mysql.gpg] http://repo.mysql.com/apt/ubuntu/ jammy mysql-8.0" \
      > /etc/apt/sources.list.d/mysql.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends mysql-community-server \
 && rm -rf /var/lib/apt/lists/* \
 && find /var/lib/mysql -mindepth 1 -delete \
 && chown mysql:mysql /var/lib/mysql \
 && rm -f /usr/sbin/policy-rc.d

# ---------- 6. Redis ----------
# Ubuntu 仓库的 redis-server 已够用，与项目配置一致监听 6380（见 entrypoint.sh）
RUN printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d \
 && chmod +x /usr/sbin/policy-rc.d \
 && export http_proxy="${PROXY}" https_proxy="${PROXY}" \
 && apt-get update \
 && apt-get install -y --no-install-recommends redis-server \
 && rm -rf /var/lib/apt/lists/* \
 && rm -f /usr/sbin/policy-rc.d

# ---------- 7. 脚本与配置模板 ----------
COPY deploy/docker/scripts /opt/xiguchat/scripts
COPY deploy/docker/config-templates /opt/xiguchat/config-templates
RUN chmod +x /opt/xiguchat/scripts/*.sh \
 && mkdir -p /var/log/xigu /workspace /var/run/mysqld /var/lib/redis \
 && ccache -M 2G

# ---------- 8. 编译校验 + 预热 ccache ----------
# 验证镜像内的工具链能编译本项目；同时填充 ccache，让容器里首次编译更快。
# 源码不留在镜像中，运行时的代码来自仓库拉取或外部挂载。
COPY . /opt/xiguchat/src
WORKDIR /opt/xiguchat/src
RUN cmake --preset linux-docker \
 && cmake --build --preset linux-docker --parallel 8 \
 && rm -rf /opt/xiguchat/src

WORKDIR /workspace
VOLUME ["/var/lib/mysql", "/workspace"]
EXPOSE 8080 8090 8091
CMD ["/opt/xiguchat/scripts/entrypoint.sh"]
