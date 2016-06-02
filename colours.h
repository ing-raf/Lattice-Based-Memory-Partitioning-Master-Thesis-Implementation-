/*
 * Macro definitions to colour the command line output
 */

#ifndef COLOURS_H
#define COLOURS_H

#define __SET_RED fprintf(stream, "\033[91m");
#define __SET_GREEN fprintf(stream, "\033[32m");
#define __SET_MAGENTA fprintf(stream, "\033[95m");
#define __SET_YELLOW fprintf(stream, "\033[93m");
#define __SET_BLUE fprintf(stream, "\033[94m");
#define __SET_DEFAULT fprintf(stream, "\033[0m");

#endif /* COLOURS_H */
