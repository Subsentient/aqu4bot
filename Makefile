all:
	$(MAKE) -C src all
clean:
	rm -fv aqu4bot aqu4bot.exe
	$(MAKE) -C src clean
	$(MAKE) -C win32 clean
install:
	@echo "There is no installation for aqu4bot."
