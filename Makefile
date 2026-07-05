CXX      := g++
CXXFLAGS := -Wall -Wextra -pedantic -std=c++17 -I.
# É necessário linkar com a biblioteca Crypto++ para usar a função de checksum
LDLIBS   := -lcryptopp

APP := ext4_app

SRCS := main.cpp \
        ext4/context.cpp \
        ext4/inode.cpp \
        ext4/dir.cpp \
        ext4/bitmap.cpp \
        parsers/superblock_parser.cpp \
        parsers/block_group_descriptor_parser.cpp \
        parsers/inode_parser.cpp \
        parsers/dir_entry_parser.cpp \
        shell/shell.cpp \
        shell/commands_read.cpp \
        shell/commands_write.cpp \
        checksum/ext4checksum.cc

OBJS := $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS))) \
        $(patsubst %.cc,%.o,$(filter %.cc,$(SRCS)))

HEADERS := $(wildcard *.h ext4/*.h parsers/*.h shell/*.h checksum/*.h)

.PHONY: all clean

all: $(APP)

$(APP): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(APP) $(OBJS)
