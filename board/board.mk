# List of all the board related files.
BOARDSRC = $(realpath $(BOARD_PATH)/board.c)

# Required include directories
BOARDINC = $(realpath $(BOARD_PATH))

ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)