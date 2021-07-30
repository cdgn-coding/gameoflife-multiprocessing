build:
	${CC} ${CFLAGS} -o main main.c

clean:
	rm -rf main

dev:
	docker build -t gameoflife .
	docker run gameoflife $(FILE)

run:
	./main 4 3 2 ./fixtures/trinomio1