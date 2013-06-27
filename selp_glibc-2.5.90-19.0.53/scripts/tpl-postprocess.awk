# Convert a file that has been passed through tpl-preprocess.awk and
# the preprocessor.  We look for:
#
#   @regexp FOO
#	Drop lines from files that do not match FOO.  Effective until
#	the next @regexp directive.
#
#   @CROSS_FOO
#	Convert to #define FOO (where FOO is any sequence of characters).
#
# Other @ characters are replaced with "#".
BEGIN {
  regexp = "."
  file = ""
  printit = 0
}
{
  if ($1 == "#" || $1 == "@regexp") {
    if ($1 == "#")
      file = $3
    else
      regexp = $2
    printit = file ~ regexp
  } else if (printit) {
    sub("^@CROSS_", "#define ", $0)
    sub("^@", "#", $0)
    print
  }
}
