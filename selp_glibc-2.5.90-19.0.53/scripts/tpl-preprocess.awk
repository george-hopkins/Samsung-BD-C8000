# Convert a *.tpl file into a preprocessor input file.  The directives
# we look for are:
#
#   @include_leaf FOO
#	Include FOO and copy lines from files ending in FOO to the output.
#	Drop lines from other files.
#
#   @copy PROTOTYPE
#	Copy the definition of a preprocessor macro to the output file.
#	PROTOTYPE is the macro's prototype, including parantheses and
#	argument names where appropriate.
#
# Other @ commands are passed through to tpl-postprocess.awk.
BEGIN {
  # We do not want the target's definition of internal_function.
  printf "#undef internal_function\n"
  printf "#define internal_function\n"
  printf "@regexp \".*%s\"\n", mainbase
}
{
  if ($1 == "@include_leaf") {
    printf "@regexp \".*%s\"\n", $2
    printf "#include <%s>\n", $2
    printf "@regexp \".*%s\"\n", mainbase
  } else if ($1 == "@copy") {
    prototype = $0
    sub(/^@copy /, "", prototype)

    name = prototype
    sub(/\(.*/, "", name)

    printf "#ifdef %s\n", name
    printf "@CROSS_%s %s\n", prototype, prototype
    printf "#endif\n"
  } else {
    print
  }
}
