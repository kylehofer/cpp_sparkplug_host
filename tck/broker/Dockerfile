# HiveMQ CE with the Eclipse Sparkplug TCK extension installed.
#   docker build -t cpp-sparkplug-host-tck tck/broker
# Both versions are pinned so an upstream change cannot alter test results unannounced.
ARG HIVEMQ_VERSION=2025.5

# ---- Build the TCK extension from source at a pinned ref ----
# Gradle 7.6 (the TCK's wrapper) does not run on Java 21, so build on 17
FROM eclipse-temurin:17-jdk AS tck-build
ARG TCK_REF=v3.0.0
RUN apt-get update \
    && apt-get install -y --no-install-recommends git unzip python3 \
    && rm -rf /var/lib/apt/lists/*
RUN git clone --depth 1 --branch ${TCK_REF} https://github.com/eclipse-sparkplug/sparkplug.git /src
WORKDIR /src
# The TCK's build includes ../specification, so the whole repository is needed. tck/gradlew
# looks for a wrapper jar that only exists at the repository root, so use the root wrapper.
RUN ./gradlew --no-daemon -p tck hivemqExtensionZip \
    && mkdir -p /out \
    && unzip -q tck/build/hivemq-extension/sparkplug-tck-*.zip -d /out

# ---- Broker ----
FROM hivemq/hivemq-ce:${HIVEMQ_VERSION}
COPY --from=tck-build /out/ /opt/hivemq/extensions/
COPY config.xml /opt/hivemq/conf/config.xml
# Results are published on SPARKPLUG_TCK/RESULT (the runner collects them from there) and also
# appended to /opt/hivemq/SparkplugTCKresults.log inside the container.
EXPOSE 1883 8000
