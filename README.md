# MiniJava_Compiler
Grammar:

Program  : MainClass 

MainClass : class id { StaticVarDecl* StaticMethodDecl* public static void main "(" String [] id ")"
               { Statement* }}
 
           
VarDecl : Type id (= Exp)? (, id (= Exp)? )* ;

StaticVarDecl : private static Type id (= Exp)? (, id (= Exp)? )* ;

StaticMethodDecl : public static Type id "(" FormalList? ")"
               {Statement*}

FormalList : Type id (, Type id)*

PrimeType : int
          : boolean
	        : String

Type : PrimeType
     : Type [ ]

Statement : VarDecl
	        : { Statement* }
          : if "(" Exp ")" Statement else Statement
          : while "(" Exp ")" Statement
          : System.out.println "(" Exp ")" ;
          : System.out.print "(" Exp ")" ;
	        : Integer.ParseInt "(" Exp ")" ;
          : LeftValue = Exp ;
	        : return Exp ;
	        : MethodCall ;

MethodCall : id "(" ExpList? ")" 

Exp : Exp op Exp
    : ! Exp
    : + Exp       
    : - Exp
    : "(" Exp ")"
	  : LeftValue
    : LeftValue . length
    : INTEGER_LITERAL
	  : STRING_LITERAL
    : true
    : false
    : MethodCall
	  : new PrimeType Index

Index :  [ Exp ]
      : Index [ Exp ]
      
ExpList : Exp (, Exp )*

LeftValue : id
	        : LeftValue [ Exp  ]
          
 ###############################################################################
 For this project, it inlcudes parsing, typechecking, intermediate code generation
 and optimization using graph coloring technique, but assuming no need to spill.
 
 This project assume array will either be one or two dimension, and method will have at
 most 4 arguments.
 
 P5_with_optimiztion: doesn't deal with array. 
 
 P5_without_optimization: implement all grammar rules but with many ldr str instruction.
                      
