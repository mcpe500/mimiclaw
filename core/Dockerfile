FROM gcc:12-alpine AS builder
RUN apk add --no-cache curl sqlite-dev sqlite-static curl-dev curl-static
WORKDIR /build
COPY shims/ ./shims/
COPY main.c .
COPY Makefile .
RUN mkdir -p vendor/cJSON && \
    curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h -o vendor/cJSON/cJSON.h && \
    curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c -o vendor/cJSON/cJSON.c
RUN make

FROM alpine:3.19
RUN apk add --no-cache libcurl sqlite-libs ca-certificates
COPY --from=builder /build/mimiclaw /usr/local/bin/
EXPOSE 18789
VOLUME /data
WORKDIR /data
ENTRYPOINT ["mimiclaw"]
