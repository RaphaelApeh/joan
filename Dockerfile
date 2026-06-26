FROM gcc:lastest

RUN apt-get update && apt-get install \
    -y cmake make

WORKDIR /app
COPY . .

RUN cmake -B build
RUN cmake --build build

# TODO