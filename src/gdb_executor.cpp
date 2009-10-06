#include "gdb_executor.h"

#include <cassert>

#include <sdk.h>
#include <loggers.h>
#include <logmanager.h>
#include <manager.h>
#include <debuggermanager.h>
#include <pipedprocess.h>

namespace
{

void GetChildPIDs(int parent, std::vector<int> &childs)
{
#ifndef __WX_MSW__
    const char *c_proc_base = "/proc";

    DIR *dir = opendir(c_proc_base);
    if(!dir)
        return;

    struct dirent *entry;
    do
    {
        entry = readdir(dir);
        if(entry)
        {
            int pid = atoi(entry->d_name);
            if(pid != 0)
            {
                char filestr[PATH_MAX + 1];
                snprintf(filestr, PATH_MAX, "%s/%d/stat", c_proc_base, pid);

                FILE *file = fopen(filestr, "r");
                if(file)
                {
                    char line[101];
                    fgets(line, 100, file);
                    fclose(file);
                    int dummy;
                    char comm[100];
                    int ppid;
                    int count = sscanf(line, "%d %s %c %d", &dummy, comm, (char *) &dummy, &ppid);
                    if(count == 4)
                    {
                        if(ppid == parent)
                        {
                            childs.push_back(pid);
                        }
                    }
                }
            }
        }
    } while(entry);

    closedir(dir);
#endif
}
}

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
    m_stopped(false),
    m_interupting(false),
    m_temporary_interupt(false)
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

long GDBExecutor::GetChildPID()
{
    if(m_pid <= 0)
        m_child_pid = -1;
    else if(m_child_pid <= 0)
    {
        std::vector<int> children;
        GetChildPIDs(m_pid, children);

        if(children.size() != 0)
        {
            if(children.size() > 1)
                Manager::Get()->GetLogManager()->Log(wxT("the debugger has more that one child"));
            m_child_pid = children.front();
        }
    }

    return m_child_pid;
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
    if(flag)
        m_interupting = false;
    else
        m_temporary_interupt = false;
}

void GDBExecutor::Interupt(bool temporary)
{
    if(!IsRunning() || IsStopped())
        return;

    if(m_logger)
        m_logger->Debug(wxT("Interupting debugger"));

    // FIXME (obfuscated#): do something similar for the windows platform
    // non-windows gdb can interrupt the running process. yay!
    if(m_pid <= 0) // look out for the "fake" PIDs (killall)
    {
        cbMessageBox(_("Unable to stop the debug process!"), _("Error"), wxOK | wxICON_WARNING);
        return;
    }
    else
    {
        m_temporary_interupt = temporary;
        m_interupting = true;
        wxKillError error;
        GetChildPID();
        wxKill(m_child_pid, wxSIGINT, &error);
        return;
    }
}

void GDBExecutor::ForceStop()
{
    if(!IsRunning())
        return;
    // FIXME (obfuscated#): do something similar for the windows platform
    // non-windows gdb can interrupt the running process. yay!
    if(m_pid <= 0) // look out for the "fake" PIDs (killall)
    {
        cbMessageBox(_("Unable to stop the debug process!"), _("Error"), wxOK | wxICON_WARNING);
        return;
    }
    else
    {
        wxKillError error;
        wxKill(m_pid, wxSIGINT, &error);

        Execute(wxT("-gdb-exit"));
        return;
    }
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
        if(!m_stopped)
        {
            if(m_logger)
            {
                m_logger->Debug(wxString::Format(wxT("GDBExecutor is not stopped, but command (%s) was executed!"),
                                                 cmd.c_str())
                                );
            }
        }
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
