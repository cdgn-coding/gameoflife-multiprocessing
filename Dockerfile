FROM gcc:4.9
COPY . /usr/src/gameoflife
WORKDIR /usr/src/gameoflife
RUN make build
ENTRYPOINT ["./main"]