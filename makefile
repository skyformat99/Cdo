cdo: src/cdo.c
	mkdir -p dist
	gcc --std=c99 -I ./src/ -o dist/cdo src/cdo.c -lncurses

clean:
	rm -rf ./dist/*
