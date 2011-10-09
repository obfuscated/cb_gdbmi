#include "helpers.h"

#include <string.h>
#include <stdio.h>

namespace dbg_mi
{
int ParseParentPID(const char *line)
{
    const char *p = strchr(line, '(');
    if (!p)
        return -1;
    ++p;
    int open_paren_count = 1;
    while (*p && open_paren_count > 0)
    {
        switch (*p)
        {
        case '(':
            open_paren_count++;
            break;
        case ')':
            open_paren_count--;
            break;
        }

        ++p;
    }
    if (*p == ' ')
        ++p;
    int dummy;
    int ppid;
    int count = sscanf(p, "%c %d", (char *) &dummy, &ppid);
    return count == 2 ? ppid : -1;
}

} // namespace dbg_mi
