#ifndef _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
#define _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_

#include "cmd_queue.h"

class PipedProcess;
class wxEvtHandler;

namespace dbg_mi
{
class LogPaneLogger : public Logger
{
public:

    virtual void Debug(wxString const &line);
    virtual wxString GetDebugLine(int index) const
    {
        return wxEmptyString;
    }
};

class GDBExecutor : public CommandExecutor
{
    GDBExecutor(GDBExecutor &o);
    GDBExecutor& operator =(GDBExecutor &o);
public:
    GDBExecutor();

    void Init(int log_page, int debug_page);
    int LaunchProcess(wxString const &cmd, wxString const& cwd, int id_gdb_process, wxEvtHandler *event_handler);

    bool ProcessHasInput();
    bool IsRunning() const;
    bool IsStopped() const { return m_stopped; }

    void Stopped(bool flag);

    virtual wxString GetOutput();

protected:
    virtual bool DoExecute(dbg_mi::CommandID const &id, wxString const &cmd);
    virtual void DoClear();
private:
    PipedProcess *m_process;
    int m_log_page;
    int m_debug_page;
    long m_pid, m_child_pid;

    bool m_stopped;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
