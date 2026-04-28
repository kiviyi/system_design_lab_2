FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

WORKDIR /app

COPY . .

RUN test -f third_party/userver/CMakeLists.txt || \
    (echo "third_party/userver is missing; run: git submodule update --init --recursive" && false)

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel 2

EXPOSE 8080

CMD ["./build/fitness-tracker-userver", "--config", "configs/static_config.yaml", "--config_vars", "configs/config_vars.yaml"]
