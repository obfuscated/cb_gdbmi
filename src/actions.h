#ifndef _DEBUGGER_GDB_MI_ACTIONS_H_
#define _DEBUGGER_GDB_MI_ACTIONS_H_

#include "cmd_queue.h"

class cbDebuggerPlugin;

namespace dbg_mi
{

class Breakpoint;

class BreakpointAddAction : public Action
{
public:
    BreakpointAddAction(Breakpoint *breakpoint) :
        m_breakpoint(breakpoint)
    {
    }
    virtual void Start();
    virtual void OnCommandResult(int32_t cmd_id);

private:
    Breakpoint *m_breakpoint;
    int m_initial_cmd;
};

class RunAction : public Action
{
public:
    RunAction(cbDebuggerPlugin *plugin, const wxString &command, bool &is_stopped);

    virtual void Start();
    virtual void OnCommandResult(int32_t cmd_id);
private:
    cbDebuggerPlugin *m_plugin;
    wxString m_command;
    bool &m_is_stopped;
};

class WatchAction : public Action
{
public:
    WatchAction(wxString const &variable_name);

    virtual void Start();
    virtual void OnCommandResult(int32_t cmd_id);
private:
    wxString m_variable_name;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_ACTIONS_H_
