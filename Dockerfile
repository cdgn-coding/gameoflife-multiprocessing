FROM gcc:11.2.0
COPY . /usr/src/gameoflife
WORKDIR /usr/src/gameoflife
RUN make build
ENTRYPOINT ["./main"]