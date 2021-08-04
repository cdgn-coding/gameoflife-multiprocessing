build:
	${CC} ${CFLAGS} -o main main.c

clean:
	rm -rf main

dev:
	docker build -t gameoflife .
	docker run gameoflife 4 3 2 ./fixtures/profe

test:
	sudo rm -rf /tmp/*
	make build
	./main $(n) 3 2 ./fixtures/$(file)

run:
	./main $(n) 3 2 ./fixtures/$(file)