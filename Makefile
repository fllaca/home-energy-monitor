build:
	pio run

clean:
	pio run -t clean

upload:
	pio run -t upload

monitor:
	pio device monitor -b 115200

vim:
	pio project init --ide vim

