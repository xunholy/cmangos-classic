FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND="noninteractive"

ARG TIMEZONE="Etc/UTC"
ENV TZ="${TIMEZONE}"
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        tzdata \
 && ln -snf "/usr/share/zoneinfo/${TIMEZONE}" /etc/localtime \
 && echo "${TIMEZONE}" > /etc/timezone \
 && dpkg-reconfigure --frontend noninteractive tzdata \
 \
 && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        ccache \
        cmake \
        g++-14 \
        git-core \
        libboost-filesystem-dev \
        libboost-program-options-dev \
        libboost-regex-dev \
        libboost-serialization-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libmariadb-dev-compat \
        libssl-dev \
        mariadb-client \
 \
 && update-alternatives --install /usr/bin/gcc gcc \
                                  /usr/bin/gcc-14 14 \
                        --slave /usr/bin/g++ g++ \
        /usr/bin/g++-14 \
 \
 && rm -rf /var/lib/apt/lists/* \
           /tmp/*

# classic-db is the only thing we still clone at build time — it's the world
# database content, not source, and it lives in its own upstream. Pin to a
# SHA for cache-friendly reproducible builds; bump when you want newer DB
# content.
ARG DATABASE_SHA1="5476595e3dedc9a1629566aea6c3c5b3c3c28e0a"

ENV HOME_DIR="/home/mangos"
ENV MANGOS_DIR="${HOME_DIR}/mangos"
ENV DATABASE_DIR="${HOME_DIR}/classic-db"

# Mangos source comes from the build context — this Dockerfile lives in the
# xunholy/cmangos-classic fork and is built against its own tree. Modules
# (twinkmaster, attunement [includes hardcore], framework) are vendored
# under src/modules/ and core patches are real commits; the build context
# already contains everything the build needs apart from classic-db.
RUN mkdir -p "${MANGOS_DIR}" "${DATABASE_DIR}"
COPY . "${MANGOS_DIR}/"

RUN cd /tmp \
 && if [ "${DATABASE_SHA1}" = "latest" ]; \
    then \
        git clone "https://github.com/cmangos/classic-db.git" \
                --branch "master" \
                --single-branch \
                --depth 1 \
            "${DATABASE_DIR}"; \
    else \
        git clone "https://github.com/cmangos/classic-db.git" \
                --branch "master" \
                --single-branch \
            cmangos-db \
     && cd cmangos-db \
     && git archive "${DATABASE_SHA1}" | tar xC "${DATABASE_DIR}"; \
    fi \
 && rm -rf /tmp/*

ARG THREADS="1"
# Sanitizer toggle. Empty (default) = Release build. Set to "address" for
# AddressSanitizer instrumentation — used for tracking down UAF/heap
# corruption in playerbots / SQL-pipeline code paths under bot load.
# Pairs with the CI job `build-builder-asan` in .github/workflows/build.yaml
# which publishes the ASan variant as `:asan-<sha>` and `:asan-latest` so
# we can roll a sanitizer build to PTR for diagnostics without rebuilding.
ARG SANITIZER=""

# ccache state lives in a BuildKit cache mount (persisted across GHA runs
# via reproducible-containers/buildkit-cache-dance in .github/workflows/
# build.yaml). On a warm cache the .cpp -> .o step short-circuits to
# previously-cached object files; only files whose preprocessed input
# changed actually recompile. Sized at 5G to comfortably hold a full
# build's object set with room for one source-tree shift. Sanitizer
# builds use a separate mount id so they don't pollute the Release
# object cache (different -f flags = different .o content).
RUN --mount=type=cache,id=cmangos-ccache${SANITIZER:+-${SANITIZER}},target=/root/.ccache,sharing=locked \
    export CCACHE_DIR=/root/.ccache \
 && export CCACHE_MAXSIZE=5G \
 && export CCACHE_COMPRESS=1 \
 && export CCACHE_COMPRESSLEVEL=6 \
 && ccache --zero-stats >/dev/null \
 \
 && SAN_CFLAGS="" \
 && SAN_LDFLAGS="" \
 && if [ -n "${SANITIZER}" ]; then \
        echo "Sanitizer build: ${SANITIZER}"; \
        # Keep Release optimization (-O2) — Debug mode (-D DEBUG=1) trips
        # a pre-existing upstream bug at WorldRunnable.cpp:99 where
        # std::atomic<unsigned int> is passed by value to a varargs
        # outString call; Release elides the copy, Debug refuses to
        # compile. Production ASan builds typically use Release + ASan
        # flags + frame pointers anyway — stack traces are still good
        # because -fno-omit-frame-pointer is explicit and -g adds
        # debug info on top of -O2.
        # Also keep PCH=1: vendored playerbots' botpch.h transitively
        # provides <regex> for PlayerbotMgr.cpp.
        SAN_CFLAGS="-fsanitize=${SANITIZER} -fno-omit-frame-pointer -g"; \
        SAN_LDFLAGS="-fsanitize=${SANITIZER}"; \
    fi \
 \
 && mkdir -p "${HOME_DIR}/build" \
             "${HOME_DIR}/run" \
 \
 && cd "${HOME_DIR}/build" \
 && cmake ../mangos/ \
        -D CMAKE_INSTALL_PREFIX=../run \
        -D CMAKE_C_COMPILER_LAUNCHER=ccache \
        -D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -D CMAKE_C_FLAGS="${SAN_CFLAGS}" \
        -D CMAKE_CXX_FLAGS="${SAN_CFLAGS}" \
        -D CMAKE_EXE_LINKER_FLAGS="${SAN_LDFLAGS}" \
        -D DEBUG=0 \
        -D PCH=1 \
        -D BUILD_AHBOT=ON \
        -D BUILD_EXTRACTORS=ON \
        -D BUILD_METRICS=ON \
        -D BUILD_PLAYERBOTS=ON \
        -D BUILD_SCRIPTDEV=ON \
        -D BUILD_MODULE_TWINKMASTER=ON \
        -D BUILD_MODULE_ATTUNEMENT=ON \
        -D BUILD_MODULE_VIP=ON \
        -D BUILD_MODULE_AUTOSCALE=ON \
        -D BUILD_MODULE_TRANSMOG=ON \
        -D BUILD_MODULE_BARBER=ON \
        -D BUILD_MODULE_TRAININGDUMMIES=ON \
        -D BUILD_MODULE_ACHIEVEMENTS=ON \
        -D BUILD_MODULE_PALADINPOWER=ON \
 \
 && make -j "${THREADS}" \
 && make install \
 \
 && ccache --show-stats \
 \
 && cd "${HOME_DIR}/run/bin/tools" \
 && chmod +x ExtractResources.sh \
             MoveMapGen.sh

RUN useradd --comment "MaNGOS" \
            --home "${HOME_DIR}" \
            --user-group mangos

WORKDIR "${HOME_DIR}"

ENV MYSQL_SUPERUSER="root"
ENV MYSQL_SUPERPASS=""

ENV MANGOS_DBHOST="host.docker.internal"
ENV MANGOS_DBPORT="3306"
ENV MANGOS_DBUSER="mangos"
ENV MANGOS_DBPASS=""

ENV MANGOS_WORLD_DBNAME="classicmangos"
ENV MANGOS_CHARACTERS_DBNAME="classiccharacters"
ENV MANGOS_LOGS_DBNAME="classiclogs"
ENV MANGOS_REALMD_DBNAME="classicrealmd"

COPY docker/builder/entrypoint.sh /
COPY docker/builder/InstallFullDB.config "${DATABASE_DIR}/"

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]

ENV VOLUME_DIR="/home/mangos/data"
ENV TMPDIR="${VOLUME_DIR}/tmp"
VOLUME ["${VOLUME_DIR}"]

ARG COMMIT_SHA
ARG CREATE_DATE
ARG VERSION
LABEL org.opencontainers.image.title="CMaNGOS Classic Builder (Emberstone)"
LABEL org.opencontainers.image.description="CMaNGOS Classic builder image with DB tools, extractors, and full world DB. Built from xunholy/cmangos-classic — flekz-games/mangos-classic modules base with twinkmaster and attunement (hardcore amalgamated) vendored in-tree."
LABEL org.opencontainers.image.licenses="GPL-2.0"
LABEL org.opencontainers.image.version="${VERSION}"
LABEL org.opencontainers.image.revision="${COMMIT_SHA}"
LABEL org.opencontainers.image.source="https://github.com/xunholy/cmangos-classic"
LABEL org.opencontainers.image.url="https://github.com/xunholy/cmangos-classic"
LABEL org.opencontainers.image.created="${CREATE_DATE}"

LABEL "net.cmangos.classic-db.revision"="${DATABASE_SHA1}"
LABEL "net.cmangos.classic-db.source"="https://github.com/cmangos/classic-db"

FROM ubuntu:24.04 AS runner

ENV DEBIAN_FRONTEND="noninteractive"

ARG TIMEZONE="Etc/UTC"
ENV TZ="${TIMEZONE}"
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        tzdata \
 && ln -snf "/usr/share/zoneinfo/${TIMEZONE}" /etc/localtime \
 && echo "${TIMEZONE}" > /etc/timezone \
 && dpkg-reconfigure --frontend noninteractive tzdata \
 \
 && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gosu \
        libasan8 \
        libmariadb-dev \
        libssl3 \
        wait-for-it \
 \
 && rm -rf /var/lib/apt/lists/* \
           /tmp/*

ENV HOME_DIR="/home/mangos"
ENV MANGOS_DIR="/opt/mangos"
RUN useradd --home "${HOME_DIR}" --create-home \
            --comment "MaNGOS" \
            --user-group mangos

WORKDIR "${MANGOS_DIR}"

COPY --from=builder /home/mangos/run "${MANGOS_DIR}"
COPY docker/runner/entrypoint.sh /
COPY docker/runner/cores-pruner.sh /usr/local/bin/cores-pruner
COPY docker/runner/graceful-shutdown.sh /usr/local/bin/graceful-shutdown
RUN chmod +x /usr/local/bin/cores-pruner /usr/local/bin/graceful-shutdown

ENV VOLUME_DIR="/var/lib/mangos"
ENV TMPDIR="${VOLUME_DIR}/tmp"
RUN mkdir "${VOLUME_DIR}" \
 && sed -i '/^DataDir/c\DataDir = "'"${VOLUME_DIR}"'"' etc/mangosd.conf.dist

# When the image is an ASan build (BUILD_ARG SANITIZER=address), these
# env vars route sanitizer diagnostics into the cores PVC and make
# violations fatal so the kubelet reports the exit. No-op on Release
# builds (binary doesn't link libasan).
ENV ASAN_OPTIONS="abort_on_error=1:halt_on_error=1:detect_leaks=1:print_stacktrace=1:log_path=/opt/mangos/cores/asan"
ENV LSAN_OPTIONS="exitcode=0"

ENV MANGOS_DBHOST="host.docker.internal"
ENV MANGOS_DBPORT="3306"
ENV MANGOS_DBUSER="mangos"
ENV MANGOS_DBPASS=""

ENV MANGOS_WORLD_DBNAME="classicmangos"
ENV MANGOS_CHARACTERS_DBNAME="classiccharacters"
ENV MANGOS_LOGS_DBNAME="classiclogs"
ENV MANGOS_REALMD_DBNAME="classicrealmd"
# Host of the login/auth DB. Empty -> entrypoint defaults it to MANGOS_DBHOST
# (single-instance default). Set only to split the auth DB onto another host.
ENV MANGOS_REALMD_DBHOST=""

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]

EXPOSE 3724 7878 8085
VOLUME ["${VOLUME_DIR}"]

ARG COMMIT_SHA
ARG CREATE_DATE
ARG VERSION
LABEL org.opencontainers.image.title="CMaNGOS Classic Runner (Emberstone)"
LABEL org.opencontainers.image.description="CMaNGOS Classic runner image with mangosd/realmd, playerbots, and the Emberstone module suite (twinkmaster, attunement [includes amalgamated hardcore])."
LABEL org.opencontainers.image.licenses="GPL-2.0"
LABEL org.opencontainers.image.version="${VERSION}"
LABEL org.opencontainers.image.revision="${COMMIT_SHA}"
LABEL org.opencontainers.image.source="https://github.com/xunholy/cmangos-classic"
LABEL org.opencontainers.image.url="https://github.com/xunholy/cmangos-classic"
LABEL org.opencontainers.image.created="${CREATE_DATE}"

ARG DATABASE_SHA1
LABEL "net.cmangos.classic-db.revision"="${DATABASE_SHA1}"
LABEL "net.cmangos.classic-db.source"="https://github.com/cmangos/classic-db"
