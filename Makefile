



default:
	echo "Please specify a target!"

build:
	$(MAKE) -C src

test: build
	./ekanz test.py

clean:
	$(MAKE) -C src clean
