# Multi-stage Dockerfile using CMakePresets (Release) and vcpkg (target: yobot_paintpp)
# - Use CMake configure preset "Release" (preset sets binaryDir=${sourceDir}/build)
# - Download runtime resources into /src/icon and /src/font
# - Build the target yobot_paintpp and collect build-time shared libraries from /src/build
# - Create non-root user 'appuser' in runtime image, chown runtime files to that user
# - Expose TCP port 9540

FROM debian:trixie AS builder

ARG DEBIAN_FRONTEND=noninteractive
ARG VCPKG_TRIPLET=x64-linux
ARG CONFIGURE_PRESET=Release
ARG TARGET_NAME=yobot_paintpp

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential ca-certificates curl git zip unzip tar \
        pkg-config cmake ninja-build autoconf autoconf-archive automake \
        libtool libltdl-dev findutils wget \
        libspdlog-dev nlohmann-json3-dev libtbb-dev openssl libcpp-httplib-dev \
        libsdl3-dev libsdl3-image-dev libsdl3-ttf-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy project source
COPY . /src

# Download runtime assets into the source tree so they end up in the final image
RUN mkdir -p /src/icon /src/font && \
    curl -fSL "https://redive.estertion.win/icon/unit/000000.webp" -o /src/icon/000000.webp && \
    curl -fSL "https://github.com/jsntn/webfonts/raw/refs/heads/master/NotoSansSC-Regular.ttf" -o /src/font/NotoSansSC-Regular.ttf

WORKDIR /src/build

# Configure using CMakePresets (Release preset sets binaryDir to ${sourceDir}/build)
RUN cmake -DCMAKE_BUILD_TYPE=${CONFIGURE_PRESET} -S /src || (echo "cmake configure preset failed" && false)

# Build the specific target
RUN cmake --build /src/build --target ${TARGET_NAME} -- -j$(nproc)

# Copy the produced executable to /dist
RUN mkdir -p /dist && \
    cp "$(find /src/build -type f -executable -name ${TARGET_NAME} -print -quit)" /dist/ || (echo "built executable not found" && false)

# ---------- Runtime ----------
FROM debian:trixie-slim AS runtime
ARG CMAKE_INSTALL_PREFIX=/usr/local
ARG TARGET_NAME=yobot_paintpp

RUN apt-get update && apt-get upgrade -y && apt-get autoremove -y && \
    apt-get install -y --no-install-recommends ca-certificates libstdc++6 adduser \
        libtbb12 openssl libcpp-httplib0.18 libsdl3-0 libsdl3-image0 libsdl3-ttf0 && \
    apt-get clean && \
    cp /usr/share/zoneinfo/Asia/Shanghai /etc/localtime && \
    echo 'Asia/Shanghai' > /etc/timezone

# Copy executable
COPY --from=builder /dist/${TARGET_NAME} ${CMAKE_INSTALL_PREFIX}/bin/${TARGET_NAME}

# Copy runtime resources
COPY --from=builder /src/icon /app/icon
COPY --from=builder /src/font /app/font

# Create non-root user and ensure ownership/permissions
RUN adduser --disabled-password --gecos "" appuser || true && \
    mkdir -p /home/appuser && \
    chown -R appuser:appuser /app ${CMAKE_INSTALL_PREFIX}/bin/${TARGET_NAME} /usr/local/lib && \
    chmod +x ${CMAKE_INSTALL_PREFIX}/bin/${TARGET_NAME}

ENV PATH=${CMAKE_INSTALL_PREFIX}/bin:$PATH \
    SDL_VIDEODRIVER=dummy
WORKDIR /app

# Expose the requested port
EXPOSE 9540

# Run as non-root
USER appuser

# Default command
CMD ["yobot_paintpp"]
