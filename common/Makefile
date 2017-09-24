# Build the LIN definitions
#

OUTPUTS			 = lin_defs.cpp lin_defs.h
INPUTS			 = ncf.peg nodes.ncf ncf_parser

${OUTPUTS}: ${INPUTS}
	./ncf_parser nodes.ncf
