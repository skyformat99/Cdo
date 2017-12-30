cdo: src/cdo.c
	gcc -I ./src/ -o dist/cdo src/cdo.c -lncurses

clean:
	rm -rf ./dist/*

dev:
	make cdo && ./dist/cdo
