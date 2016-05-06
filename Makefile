TARGET=LSTDAQ
CXX=g++
VPATH=../include
INCDIR = ./include
LDFLAGS = -lrt -lpthread
OBJDIR=obj
SRCDIR=src
INCLUDE=-I./include
SOURCES   = $(wildcard $(SRCDIR)/*.cpp)
OBJS=$(addprefix $(OBJDIR)/, $(notdir $(SOURCES:.cpp=.o)))
all:$(TARGET) Dox

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o:$(SRCDIR)/%.cpp
	-mkdir -p $(OBJDIR)
	$(CXX) $(INCLUDE) -o $@ -c $^ 

Dox:
	 doxygen Doxyfile
.PHONY: clean Dox

clean:
	$(RM) $(OBJS) $(TARGET) 
	mv $(SRCDIR)/*~ bk/
	mv $(INCLUDEDIR)/*~ bk/