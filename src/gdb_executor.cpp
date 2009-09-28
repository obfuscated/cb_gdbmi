#include "gdb_executor.h"

#include <cassert>

namespace dbg_mi
{

wxString GDBExecutor::GetOutput()
{
    assert(false);
    return wxEmptyString;
}

bool GDBExecutor::DoExecute(dbg_mi::CommandID const &id, wxString const &cmd)
{
    return false;
}


} // namespace dbg_mi
