CC=arduino
CFLAGS=
BOARD_STRING=default

default:
	@echo "Usage: make [verify/upload] BOARD=[mega/uno/nano] [PORT=/dev/ACM?] MODULE=folder"

# Compile MODULE
verify: check-BOARD parse-board check-MODULE
	$(CC) --verify --board $(BOARD_STRING) $(MODULE)/$(MODULE).ino

# Compile and upload MODULE
upload: check-BOARD parse-board check-PORT check-MODULE
	$(CC) --upload --board $(BOARD_STRING) --port $(PORT) $(MODULE)/$(MODULE).ino

# Parse BOARD to define BOARD_STRING
parse-board:
ifeq ($(BOARD), nano) # nano needs special settings
    BOARD_STRING="arduino:avr:nano:cpu=atmega328"
else
    BOARD_STRING="arduino:avr:$(BOARD)"
endif

# Check if an env variable is defined
# https://stackoverflow.com/a/7367903/5135869
check-%:
	@if [ "${${*}}" = "" ]; then                 \
            echo "Environment variable $* not set"; \
            exit 1;                                 \
        fi
