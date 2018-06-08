find_package( Doxygen )

set( DOXYGEN_FILE_PATTERNS "*.h" )
set( DOXYGEN_GENERATE_HTML NO )
set( DOXYGEN_GENERATE_MAN YES )

# There are no classes in C,
#  Doxygen treating C as C++ by detecting structs as classes
#   1.  To use structs and classes sysnonymusly in C++ is a bad
#        practice because it not only confuses other novice
#        programers into thinking they're the same but is an abuse
#        of C++ such that the security private members is bypassed
#        and devalued.  Also, by giving classes member fuctions (the
#        core feature seperating classes form structs) one violates
#        the age-old concepts of programming that make the versital
#        application of structures/records and unions possable.  If
#        you do not know what these are, you probably got caught in
#        the trap of using higher level languges and will have to
#        unlearn everything you think you know about computer
#        software infastructure in order to grow as a developer
#        starting with pointers and memory allocation and leading
#        into typing.
#   2.  Despite the location of the struct definition which makes
#        it's internals available to other parts of the program,
#        user interaction of these components is not recomended.
#        This oversight may require remedy, but this kind of
#        restructuring is a low priority compared to the completion
#        of the code.
#   3.  If comprehension of the structs is needed, it should be
#        attained by reading the code and not by the summary that is
#        the end-user's documentation.
set( DOXYGEN_HIDE_UNDOC_CLASSES YES )

doxygen_add_docs( userDocs )

