#!
#! This is a custom template for creating a Makefile for the moc.
#!
#$ IncludeTemplate("app.t");

####### Lex/yacc programs and options

LEX	=	flex
YACC	=	#$ $text = ($is_unix ? "yacc -d" : "byacc -d");

####### Lex/yacc files

LEXIN	=	#$ Expand("LEXINPUT");
LEXOUT  =	lex.yy.c
YACCIN	=	#$ Expand("YACCINPUT");
YACCOUT =	y.tab.c
YACCHDR =	y.tab.h
MOCGEN  =	#$ Expand("MOCGEN");

####### Process lex/yacc files

$(LEXOUT): $(LEXIN)
	$(LEX) $(LEXIN)

$(MOCGEN): moc.y $(LEXOUT)
	$(YACC) moc.y
	#$ $text = ($is_unix ? "-rm -f " : "-del ") . '$(MOCGEN)';
	#$ $text = ($is_unix ? "-mv " : "-ren ") . '$(YACCOUT) $(MOCGEN)'; 
