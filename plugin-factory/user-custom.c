#include "parser.h"

/*
 * NOP PARSER
 *
 * This is a skeleton parser. Nothing has been implemented yet.
 * You can invoke this parser by the following command.
 *
 * $ gtags --gtagsconf=/usr/local/share/gtags/gtags.conf --gtagslabel=user
 */
void
parser(const struct parser_param *param)
{
	param->warning("nop parser: %s", param->file);
}
