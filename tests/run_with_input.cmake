# Runs PROGRAM with standard input read from INPUT, as in
# "program < input", and passes on its output. ctest's add_test can't
# redirect input itself.
execute_process(COMMAND ${PROGRAM} INPUT_FILE ${INPUT}
                RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
message("${out}${err}")
if(NOT status EQUAL 0)
  message(FATAL_ERROR "${PROGRAM} exited with status ${status}")
endif()
