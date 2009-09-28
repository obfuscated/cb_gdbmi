#ifndef _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
#define _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_

#include "cmd_queue.h"

namespace dbg_mi
{
class GDBExecutor : public CommandExecutor
{
public:
    virtual wxString GetOutput();

protected:
    virtual bool DoExecute(dbg_mi::CommandID const &id, wxString const &cmd);

};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
