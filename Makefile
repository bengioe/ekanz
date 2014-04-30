



default:
	@echo "Please specify a target!"

build:
	$(MAKE) -C src

test: build
	./ekanz while_test.py

clean:
	$(MAKE) -C src clean
