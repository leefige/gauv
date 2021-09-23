parser grammar MPCParser;
options {
	tokenVocab = MPCLexer;
}

/*Basic concepts*/

translationUnit: declarationseq? EOF;

/*Expressions*/

declarationseq: declaration+;

declaration:
	emptyDeclaration;

emptyDeclaration: Semi;
