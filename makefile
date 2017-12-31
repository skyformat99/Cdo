cdo: src/cdo.c
	gcc --std=c99 -I ./src/ -o dist/cdo src/cdo.c -lncurses

run:
	./dist/cdo

clean:
	rm -rf ./dist/*

dev:
	make cdo && make run

