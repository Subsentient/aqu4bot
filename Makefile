all:
	$(MAKE) -C src all
clean:
	rm -fv aqu4bot aqu4bot.exe
	$(MAKE) -C src clean
install:
	@echo "There is no installation for aqu4bot."
