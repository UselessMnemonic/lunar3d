get_filename_component(INPUT_NAME ${INPUT} NAME)
string(REPLACE "." "_" SYMBOL ${INPUT_NAME})
string(REGEX REPLACE "^([0-9])" "_\\1" SYMBOL ${SYMBOL})

file(WRITE ${OUTPUT} "extern const unsigned char ${SYMBOL}_end[];\n")
file(APPEND ${OUTPUT} "extern const unsigned char ${SYMBOL}[];\n")
file(APPEND ${OUTPUT} "extern const unsigned int ${SYMBOL}_size;\n")