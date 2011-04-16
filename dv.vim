" Vim syntax file
" Language:	deva
" Maintainer:	Josh Shepard <josh.shepard@gmail.com>
" Last Change:	2010 Dec 09 
" loosely based on awk,c

" cindent works well with deva if you set cinkeys to: 0{,0},0),:,!^F,o,O,e
" (i.e. don't indent to col 0 on '#' key, which is a comment in deva, 
" not a preproc statement)


" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syn clear
elseif exists("b:current_syntax")
  finish
endif

" keywords
syn keyword devaStatement	break continue 
syn keyword devaStatement	def 
syn keyword devaStatement	return
syn keyword devaStatement	import
syn keyword devaStatement	local
syn keyword devaStatement	const
syn keyword devaStatement	extern
syn keyword devaFunction	print
syn keyword devaFunction	str
syn keyword devaFunction	append
syn keyword devaFunction	copy
syn keyword devaFunction	length
syn keyword devaFunction	eval
syn keyword devaFunction	concat
syn keyword devaFunction	min
syn keyword devaFunction	max
syn keyword devaFunction	pop
syn keyword devaFunction	insert
syn keyword devaFunction	remove
syn keyword devaFunction	find
syn keyword devaFunction	rfind
syn keyword devaFunction	count
syn keyword devaFunction	reverse
syn keyword devaFunction	sort
syn keyword devaFunction	map
syn keyword devaFunction	reduce
syn keyword devaFunction	filter
syn keyword devaFunction	slice
syn keyword devaFunction	keys
syn keyword devaFunction	values
syn keyword devaFunction	merge
syn keyword devaFunction	and or xor 
syn keyword devaFunction	name
syn keyword devaFunction	type
syn keyword devaBool		true false
syn keyword devaStatement	null
syn keyword devaObj		class new delete self

syn keyword devaConditional	if else
syn keyword devaRepeat	while for in

syn keyword devaTodo		contained TODO

syn match   devaFieldVars	"\$\d\+"

"catch errors caused by wrong parenthesis
syn region	devaParen	transparent start="(" end=")" contains=ALLBUT,devaParenError,devaSpecialCharacter,devaArrayElement,devaArrayArray,devaTodo,devaRegExp,devaBrktRegExp,devaBrackets,devaCharClass
syn match	devaParenError	display ")"
syn match	devaInParen	display contained "[{}]"

" 64 lines for complex &&'s, and ||'s in a big "if"
syn sync ccomment devaParen maxlines=64

" String and Character constants
" Highlight special characters (those which have a backslash) differently
syn region  devaString	start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=devaSpecialCharacter,devaSpecialPrintf
syn region  devaString	start=+'+  skip=+\\\\\|\\'+  end=+'+  contains=devaSpecialCharacter,devaSpecialPrintf
syn match   devaSpecialCharacter contained "\\."

" Some of these combinations may seem weird, but they work.
syn match   devaSpecialPrintf	contained "%[-+ #]*\d*\.\=\d*[cdefgiosuxEGX%]"

" Numbers, allowing signs (both -, and +)
" Integer number.
syn match  devaNumber		display "[+-]\=\<\d\+\>"
syn match  devaNumber		display "0x[0-9A-Fa-f]\+"
syn match  devaNumber		display "0o[0-8]\+"
syn match  devaNumber		display "0b[0-1]\+"
" Floating point number.
syn match  devaFloat		display "[+-]\=\<\d\+\.\d+\>"
" Floating point number, starting with a dot.
syn match  devaFloat		display "[+-]\=\<.\d+\>"
syn case ignore
"floating point number, with dot, optional exponent
syn match  devaFloat	display "\<\d\+\.\d*\(e[-+]\=\d\+\)\=\>"
"floating point number, starting with a dot, optional exponent
syn match  devaFloat	display "\.\d\+\(e[-+]\=\d\+\)\=\>"
"floating point number, without dot, with exponent
syn match  devaFloat	display "\<\d\+e[-+]\=\d\+\>"
syn case match

syn match  devaIdentifier	"\<[a-zA-Z_][a-zA-Z0-9_]*\>"

" Arithmetic operators: +, and - take care of ++, and --
syn match   devaOperator	"+\|-\|\*\|/\|%\|="
syn match   devaOperator	"+=\|-=\|\*=\|/=\|%="
syn match   devaOperator	"\$"

" Comparison expressions.
syn match   devaExpression	"==\|>=\|=>\|<=\|=<\|\!="
syn match   devaExpression	"\!"

" Boolean Logic (OR, AND, NOT)
syn match  devaBoolLogic	"||\|&&\|\!\||\|&"

" Expression separators: ';' and ','
syn match  devaSemicolon	";"
syn match  devaComma		","

syn match  devaComment	"#.*" contains=devaTodo

syn match  devaLineSkip	"\\$"

" define the default highlighting
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlightling yet
if version >= 508 || !exists("did_deva_syn_inits")
  if version < 508
    let did_deva_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink devaConditional	Conditional
  HiLink devaFunction		Function
  HiLink devaRepeat		Repeat
  HiLink devaStatement		Statement

  HiLink devaString		String
  HiLink devaSpecialPrintf	Special
  HiLink devaSpecialCharacter	Special

  HiLink devaNumber		Number
  HiLink devaFloat		Float
  HiLink devaBool		Statement

  HiLink devaObj		Statement

  HiLink devaFileIO		Special
  HiLink devaOperator		Special
  HiLink devaExpression		Special
  HiLink devaBoolLogic		Special

  HiLink devaPatterns		Special
  HiLink devaVariables		Special
  HiLink devaFieldVars		Special

  HiLink devaLineSkip		Special
  HiLink devaSemicolon		Special
  HiLink devaComma		Special
  HiLink devaIdentifier		Identifier

  HiLink devaComment		Comment
  HiLink devaTodo		Todo

  " Change this if you want nested array names to be highlighted.
  HiLink devaArrayArray		devaArray
  HiLink devaArrayElement	Special

  HiLink devaParenError		devaError
  HiLink devaInParen		devaError
  HiLink devaError		Error

  delcommand HiLink
endif

let b:current_syntax = "dv"

" vim: ts=8
