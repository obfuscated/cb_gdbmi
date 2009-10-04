#include "gdb_executor.h"

#include <cassert>

#include <sdk.h>
#include <loggers.h>
#include <logmanager.h>
#include <manager.h>
#include <debuggermanager.h>
#include <pipedprocess.h>

namespace dbg_mi
{

void LogPaneLogger::Debug(wxString const &line)
{
    int index;
    Manager::Get()->GetDebuggerManager()->GetLogger(true, index);
    if(index != -1)
        Manager::Get()->GetLogManager()->Log(line, index);
}

GDBExecutor::GDBExecutor() :
    m_process(NULL),
    m_log_page(-1),
    m_debug_page(-1),
    m_pid(-1),
    m_child_pid(-1),
    m_stopped(false)
{
}

int GDBExecutor::LaunchProcess(wxString const &cmd, wxString const& cwd, int id_gdb_process,
                               wxEvtHandler *event_handler)
{
    LogManager *log = Manager::Get()->GetLogManager();
    if(m_process)
        return -1;

    // start the gdb process
    m_process = new PipedProcess((void**)&m_process, event_handler, id_gdb_process, true, cwd, false);
    log->Log(_("Starting debugger: "), m_log_page);
    m_pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, m_process);
    m_child_pid = -1;

#ifdef __WXMAC__
    if (m_pid == -1)
    {
        log->LogError(_("debugger has fake macos PID"), m_log_page);
    }
#endif

    if (!m_pid)
    {
        delete m_process;
        m_process = 0;
        log->Log(_("failed"), m_log_page);
        return -1;
    }
    else if (!m_process->GetOutputStream())
    {
        delete m_process;
        m_process = 0;
        log->Log(_("failed (to get debugger's stdin)"), m_log_page);
        return -2;
    }
    else if (!m_process->GetInputStream())
    {
        delete m_process;
        m_process = 0;
        log->Log(_("failed (to get debugger's stdout)"), m_log_page);
        return -2;
    }
    else if (!m_process->GetErrorStream())
    {
        delete m_process;
        m_process = 0;
        log->Log(_("failed (to get debugger's stderr)"), m_log_page);
        return -2;
    }
    log->Log(_("done"), m_log_page);

    return 0;
}

bool GDBExecutor::ProcessHasInput()
{
    return m_process && m_process->HasInput();
}

bool GDBExecutor::IsRunning() const
{
    return m_process;
}

void GDBExecutor::Stopped(bool flag)
{
    if(m_logger)
    {
        if(flag)
            m_logger->Debug(wxT("Executor stopped"));
        else
            m_logger->Debug(wxT("Executor started"));
    }
    m_stopped = flag;
}

wxString GDBExecutor::GetOutput()
{
    assert(false);
    return wxEmptyString;
}

bool GDBExecutor::DoExecute(dbg_mi::CommandID const &id, wxString const &cmd)
{
    if(m_process)
    {
        m_process->SendString(id.ToString() + cmd);
        return true;
    }
    else
        return false;
}
void GDBExecutor::DoClear()
{
    m_stopped = true;
}

} // namespace dbg_mi
