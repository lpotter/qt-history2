#!
#! This is the tmake template for building Qt example programs
#!
####### This section is automatically generated from
#######    /home/hanord/qt/examples/Makefile

INCDIR	=	$(QTDIR)/include
CFLAGS	=	-O2
LFLAGS	=	-L$(QTDIR)/lib -lqt
CC	=	gcc
MOC	=	moc

####### End of automatically generated section
#
# $Source: /tmp/cvs/qt/tmake/linux-gcc/Attic/example.t,v $
#

#############################################################################
#$ $moc_aware = 1;
#$ StdInit();
#$ $project{"TARGET"} || ($project{"TARGET"} = "a.out");
#!
# Makefile for building #$ Expand("TARGET")
# Generated by tmake at #$ Now();
#     Project: #$ $text = $project_name;
#    Template: #$ $text = $template_name;
#############################################################################

####### Files

HEADERS =	#$ ExpandList("HEADERS");
SOURCES =	#$ ExpandList("SOURCES");
OBJECTS =	#$ ExpandList("OBJECTS");
SRCMOC	=	#$ ExpandList("SRCMOC");
OBJMOC	=	#$ ExpandList("OBJMOC");
TARGET	=	#$ Expand("TARGET");

####### Implicit rules

.SUFFIXES: .cpp .c

.cpp.o:
	$(CC) -c $(CFLAGS) -I$(INCDIR) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) -I$(INCDIR) -o $@ $<

####### Build rules

all: $(TARGET) #$ Expand("ALL_DEPS");

$(TARGET): $(OBJECTS) $(OBJMOC)
	$(CC) $(OBJECTS) $(OBJMOC) -o $(TARGET) $(LFLAGS) #$ Expand("UNIXLIBS");

moc: $(SRCMOC)

clean:
	-rm -f $(OBJECTS) $(OBJMOC) $(SRCMOC) $(TARGET)
	#$ ExpandGlue("CLEAN_FILES","-rm -f "," ","");

####### Compile

#$ BuildObj($project{"OBJECTS"},$project{"SOURCES"});
#$ BuildMocObj($project{"OBJMOC"},$project{"SRCMOC"});
#$ BuildMocSrc($project{"HEADERS"});
#$ BuildMocSrc($project{"SOURCES"});
