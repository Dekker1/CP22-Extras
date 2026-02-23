### MiniZinc Evaluator Library

add_library(libminizinc-eval SHARED
  lib/minizinc_eval.cpp
  include/minizinc/minizinc_eval.hh
)
target_link_libraries(libminizinc-eval mzn)

install(
  TARGETS libminizinc-eval
  EXPORT libminizincTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
