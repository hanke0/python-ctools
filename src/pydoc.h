#ifndef _CTOOLS_PYDOC_H_
#define _CTOOLS_PYDOC_H_

#define USUAL_SETNX_METHOD_DOC                                                 \
  "setnx(key, fn=None)\n--\n\nLike setdefault but accept a callable.\n"        \
  "\n"                                                                         \
  "Parameters\n"                                                               \
  "----------\n"                                                               \
  "key : object\n"                                                             \
  "  Hash key.\n"                                                              \
  "fn : typing.Callable[[typing.Any], typing.Any], optional\n"                 \
  "  It's a callable that accept key as only one argument, called when key "   \
  "not exists.\n"                                                              \
  "\n"                                                                         \
  "Returns\n"                                                                  \
  "-------\n"                                                                  \
  "object\n"                                                                   \
  "  The found value or what ``setnx`` return.\n"

#endif /* _CTOOLS_PYDOC_H_ */
