LEX = flex
YACC = bison

SRC = $(wildcard *.cc)

CFLAGS = -g

codegen:    lex.yy.c y.tab.c $(SRC)
			g++ $(SRC) lex.yy.c y.tab.c -g -o codegen

y.tab.c:	parser.y
			bison -v -y -d -g -t --verbose parser.y

lex.yy.c:	parser.l
			flex parser.l 

y.tab.h:	y.tab.c

clean:
	rm -f lex.yy.c y.tab.c y.tab.h y.dot y.output codegen *.s output/*.s $(OBJS) $(LYOBJS)


