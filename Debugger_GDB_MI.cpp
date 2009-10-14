#include <sdk.h> // Code::Blocks SDK

#include "Debugger_GDB_MI.h"

#include <algorithm>
#include <wx/xrc/xmlres.h>

#include <backtracedlg.h>
#include <cbproject.h>
#include <compilerfactory.h>
#include <configurationpanel.h>
#include <configmanager.h>
#include <editbreakpointdlg.h>
#include <loggers.h>
#include <logmanager.h>
#include <macrosmanager.h>
#include <pipedprocess.h>
#include <projectmanager.h>
#include <threadsdlg.h>

#include "actions.h"
#include "cmd_result_parser.h"
#include "frame.h"

#ifndef __WX_MSW__
#include <dirent.h>
#include <stdlib.h>
#endif


// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<Debugger_GDB_MI> reg(_T("debugger_gdbmi"));
}

int const id_gdb_process = wxNewId();
int const id_gdb_poll_timer = wxNewId();

// events handling
BEGIN_EVENT_TABLE(Debugger_GDB_MI, cbDebuggerPlugin)

    EVT_PIPEDPROCESS_STDOUT(id_gdb_process, Debugger_GDB_MI::OnGDBOutput)
    EVT_PIPEDPROCESS_STDERR(id_gdb_process, Debugger_GDB_MI::OnGDBOutput)
    EVT_PIPEDPROCESS_TERMINATED(id_gdb_process, Debugger_GDB_MI::OnGDBTerminated)

    EVT_IDLE(Debugger_GDB_MI::OnIdle)
    EVT_TIMER(id_gdb_poll_timer, Debugger_GDB_MI::OnTimer)

END_EVENT_TABLE()

// constructor
Debugger_GDB_MI::Debugger_GDB_MI() :
    m_menu(NULL),
    m_log(NULL),
    m_debug_log(NULL),
    m_current_thread(-1)
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if(!Manager::LoadResource(_T("debugger_gdbmi.zip")))
    {
        NotifyMissingFile(_T("debugger_gdbmi.zip"));
    }

    m_executor.SetLogger(&m_execution_logger);
}

// destructor
Debugger_GDB_MI::~Debugger_GDB_MI()
{
}

void Debugger_GDB_MI::OnAttachReal()
{
    m_timer_poll_debugger.SetOwner(this, id_gdb_poll_timer);

    DebuggerManager &dbg_manager = *Manager::Get()->GetDebuggerManager();
    dbg_manager.RegisterDebugger(this, _T("gdbmi"));

    m_log = dbg_manager.GetLogger(false, m_page_index);
    m_debug_log = dbg_manager.GetLogger(true, m_dbg_page_index);


//-    m_command_queue.SetLogPages(m_page_index, m_dbg_page_index);

    // do whatever initialization you need for your plugin
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be TRUE...
    // You should check for it in other functions, because if it
    // is FALSE, it means that the application did *not* "load"
    // (see: does not need) this plugin...
}

void Debugger_GDB_MI::OnRelease(bool appShutDown)
{
    DebuggerManager &dbg_manager = *Manager::Get()->GetDebuggerManager();
    dbg_manager.UnregisterDebugger(this);

    dbg_manager.HideLogger(false);
    dbg_manager.HideLogger(true);

    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...
}

void Debugger_GDB_MI::ShowToolMenu()
{
    #warning "not implemented"
}

int Debugger_GDB_MI::Configure()
{
    //create and display the configuration dialog for your plugin
    cbConfigurationDialog dlg(Manager::Get()->GetAppWindow(), wxID_ANY, _("Your dialog title"));
    cbConfigurationPanel* panel = GetConfigurationPanel(&dlg);
    if (panel)
    {
        dlg.AttachConfigurationPanel(panel);
        PlaceWindow(&dlg);
        return dlg.ShowModal() == wxID_OK ? 0 : -1;
    }
    return -1;
}

void Debugger_GDB_MI::ShowLog()
{
    // TODO (obfuscated#): if the "Debugger (debug)" log pane is active don't activate the "Debugger"
    CodeBlocksLogEvent event_switch_log(cbEVT_SWITCH_TO_LOG_WINDOW, m_log);
    CodeBlocksLogEvent event_show_log(cbEVT_SHOW_LOG_MANAGER);
    Manager::Get()->ProcessEvent(event_switch_log);
    Manager::Get()->ProcessEvent(event_show_log);
}

bool Debugger_GDB_MI::SelectCompiler(cbProject &project, Compiler *&compiler,
                                     ProjectBuildTarget *&target, long pid_to_attach)
{
    LogManager &log = *Manager::Get()->GetLogManager();
   // select the build target to debug
    target = NULL;
    compiler = NULL;
    wxString active_build_target;

    if(pid_to_attach == 0)
    {
        ShowLog();

        log.Log(_("Selecting target: "), m_page_index);
        if (!project.BuildTargetValid(active_build_target, false))
        {
            int tgtIdx = project.SelectTarget();
            if (tgtIdx == -1)
            {
                log.Log(_("canceled"), m_page_index);
                return false;
            }
            target = project.GetBuildTarget(tgtIdx);
            active_build_target = target->GetTitle();
        }
        else
            target = project.GetBuildTarget(active_build_target);

        // make sure it's not a commands-only target
        if (target->GetTargetType() == ttCommandsOnly)
        {
            cbMessageBox(_("The selected target is only running pre/post build step commands\n"
                        "Can't debug such a target..."), _("Information"), wxICON_INFORMATION);
            log.Log(_("aborted"), m_page_index);
            return false;
        }
        log.Log(target->GetTitle(), m_page_index);

        // find the target's compiler (to see which debugger to use)
        compiler = CompilerFactory::GetCompiler(target ? target->GetCompilerID() : project.GetCompilerID());
    }
    else
        compiler = CompilerFactory::GetDefaultCompiler();
    return true;
}


wxString Debugger_GDB_MI::FindDebuggerExecutable(Compiler* compiler)
{
    if (compiler->GetPrograms().DBG.IsEmpty())
        return wxEmptyString;
//    if (!wxFileExists(compiler->GetMasterPath() + wxFILE_SEP_PATH + _T("bin") + wxFILE_SEP_PATH + compiler->GetPrograms().DBG))
//        return wxEmptyString;

    wxString masterPath = compiler->GetMasterPath();
    while (masterPath.Last() == '\\' || masterPath.Last() == '/')
        masterPath.RemoveLast();
    wxString gdb = compiler->GetPrograms().DBG;
    const wxArrayString& extraPaths = compiler->GetExtraPaths();

    wxPathList pathList;
    pathList.Add(masterPath + wxFILE_SEP_PATH + _T("bin"));
    for (unsigned int i = 0; i < extraPaths.GetCount(); ++i)
    {
        if (!extraPaths[i].IsEmpty())
            pathList.Add(extraPaths[i]);
    }
    pathList.AddEnvList(_T("PATH"));
    wxString binPath = pathList.FindAbsoluteValidPath(gdb);
    // it seems, under Win32, the above command doesn't search in paths with spaces...
    // look directly for the file in question in masterPath
    if (binPath.IsEmpty() || !(pathList.Index(wxPathOnly(binPath)) != wxNOT_FOUND))
    {
        if (wxFileExists(masterPath + wxFILE_SEP_PATH + _T("bin") + wxFILE_SEP_PATH + gdb))
            binPath = masterPath + wxFILE_SEP_PATH + _T("bin");
        else if (wxFileExists(masterPath + wxFILE_SEP_PATH + gdb))
            binPath = masterPath;
        else
        {
            for (unsigned int i = 0; i < extraPaths.GetCount(); ++i)
            {
                if (!extraPaths[i].IsEmpty())
                {
                    if (wxFileExists(extraPaths[i] + wxFILE_SEP_PATH + gdb))
                    {
                        binPath = extraPaths[i];
                        break;
                    }
                }
            }
        }
    }

    return binPath;
}

wxString Debugger_GDB_MI::GetDebuggee(ProjectBuildTarget* target)
{
    if (!target)
        return wxEmptyString;

    wxString out;
    switch (target->GetTargetType())
    {
        case ttExecutable:
        case ttConsoleOnly:
            out = UnixFilename(target->GetOutputFilename());
            Manager::Get()->GetMacrosManager()->ReplaceEnvVars(out); // apply env vars
            Manager::Get()->GetLogManager()->Log(_("Adding file: ") + out, m_page_index);
//            ConvertToGDBDirectory(out);
            break;

        case ttStaticLib:
        case ttDynamicLib:
            // check for hostapp
            if (target->GetHostApplication().IsEmpty())
            {
                cbMessageBox(_("You must select a host application to \"run\" a library..."));
                return wxEmptyString;
            }
            out = UnixFilename(target->GetHostApplication());
            Manager::Get()->GetMacrosManager()->ReplaceEnvVars(out); // apply env vars
            Manager::Get()->GetLogManager()->Log(_("Adding file: ") + out, m_page_index);
//            ConvertToGDBDirectory(out);
            break;
//            // for DLLs, add the DLL's symbols
//            if (target->GetTargetType() == ttDynamicLib)
//            {
//                wxString symbols;
//                out = UnixFilename(target->GetOutputFilename());
//                Manager::Get()->GetMacrosManager()->ReplaceEnvVars(out); // apply env vars
//                msgMan->Log(m_page_index, _("Adding symbol file: %s"), out.c_str());
//                ConvertToGDBDirectory(out);
//                QueueCommand(new DbgCmd_AddSymbolFile(this, out));
//            }
//            break;

        default: break;
    }
    return out;
}

void Debugger_GDB_MI::OnGDBOutput(wxCommandEvent& event)
{
    wxString const &msg = event.GetString();
    if (!msg.IsEmpty())
        ParseOutput(msg);
}

void Debugger_GDB_MI::OnGDBTerminated(wxCommandEvent& event)
{
    ClearActiveMarkFromAllEditors();
    Manager::Get()->GetLogManager()->Log(_T("debugger terminated!"), m_page_index);
    m_timer_poll_debugger.Stop();
    m_actions.Clear();
    m_executor.Clear();

    // Notify debugger plugins for end of debug session
    PluginManager *plm = Manager::Get()->GetPluginManager();
    CodeBlocksEvent evt(cbEVT_DEBUGGER_FINISHED);
    plm->NotifyPlugins(evt);

    // TODO (obfuscated#): switch to the previous layout/perspective
}

void Debugger_GDB_MI::OnIdle(wxIdleEvent& event)
{
    if(m_executor.IsStopped() && m_executor.IsRunning())
    {
        m_actions.Run(m_executor);
    }
    if(m_executor.ProcessHasInput())
        event.RequestMore();
    else
        event.Skip();
}

void Debugger_GDB_MI::OnTimer(wxTimerEvent& event)
{
    RunQueue();
    wxWakeUpIdle();
}

void Debugger_GDB_MI::AddStringCommand(wxString const &command)
{
//-    Manager::Get()->GetLogManager()->Log(wxT("Queue command: ") + command, m_dbg_page_index);
//-    dbg_mi::Command *cmd = new dbg_mi::Command();
//-    cmd->SetString(command);
//-    m_command_queue.AddCommand(cmd, true);
//    m_executor.Execute(command);
    m_actions.Add(new dbg_mi::SimpleAction(command));
}

struct Notifications
{
    Notifications(Debugger_GDB_MI *plugin, dbg_mi::GDBExecutor &executor, int page_index, bool simple_mode) :
        m_plugin(plugin),
        m_page_index(page_index),
        m_executor(executor),
        m_simple_mode(simple_mode)
    {
    }

    void operator()(dbg_mi::ResultParser const &parser)
    {
        dbg_mi::ResultValue const &result_value = parser.GetResultValue();
        if(m_simple_mode)
        {
            ParseStateInfo(result_value);
            m_plugin->UpdateWhenStopped();
        }
        else
        {
            LogManager *log = Manager::Get()->GetLogManager();
            log->Log(_T("notification event recieved!"), m_page_index);

            if(parser.GetResultClass() == dbg_mi::ResultParser::ClassStopped)
            {
                dbg_mi::StoppedReason reason = dbg_mi::StoppedReason::Parse(result_value);

                if(reason == dbg_mi::StoppedReason::ExitedNormally)
                {
                    m_executor.Execute(wxT("-gdb-exit"));
                }
                else
                {
                    if(!m_executor.IsTemporaryInterupt())
                        ParseStateInfo(result_value);

                    m_executor.Stopped(true);

                    // Notify debugger plugins for end of debug session
                    PluginManager *plm = Manager::Get()->GetPluginManager();
                    CodeBlocksEvent evt(cbEVT_DEBUGGER_PAUSED);
                    plm->NotifyPlugins(evt);

                    m_plugin->UpdateWhenStopped();
                }
            }
        }
    }

private:
    void ParseStateInfo(dbg_mi::ResultValue const &result_value)
    {
        LogManager *log = Manager::Get()->GetLogManager();
        dbg_mi::Frame frame;
        if(!frame.ParseOutput(result_value))
        {
            log->Log(_T("Debugger_GDB_MI::OnGDBNotification: can't find/parse frame value:("),
                                                 m_page_index);
            log->Log(wxString::Format(wxT("Debugger_GDB_MI::OnGDBNotification: %s"),
                                      result_value.MakeDebugString().c_str()),
                     m_page_index);
        }
        else
        {
            dbg_mi::ResultValue const *thread_id_value;
            thread_id_value = result_value.GetTupleValue(m_simple_mode ? wxT("new-thread-id") : wxT("thread-id"));
            if(thread_id_value)
            {
                long id;
                if(!thread_id_value->GetSimpleValue().ToLong(&id, 10))
                {
                    log->Log(wxString::Format(wxT("Debugger_GDB_MI::OnGDBNotification ")
                                              wxT(" thread_id parsing failed (%s)"),
                                              result_value.MakeDebugString().c_str()),
                             m_page_index);
                }
                else
                    m_plugin->SetCurrentThread(id);
            }
            if(frame.HasValidSource())
            {
                DebuggerManager *dbg = Manager::Get()->GetDebuggerManager();
                dbg->SyncEditor(frame.GetFilename(), frame.GetLine(), true);
            }
        }
    }

private:
    Debugger_GDB_MI *m_plugin;
    int m_page_index;
    dbg_mi::GDBExecutor &m_executor;
    bool m_simple_mode;
};

void Debugger_GDB_MI::UpdateWhenStopped()
{
    DebuggerManager *dbg_manager = Manager::Get()->GetDebuggerManager();
    if(IsWindowReallyShown(dbg_manager->GetBacktraceDialog()))
    {
        RequestUpdate(Backtrace);
    }

    if(IsWindowReallyShown(dbg_manager->GetThreadsDialog()))
    {
        RequestUpdate(Threads);
    }
}

void Debugger_GDB_MI::SetCurrentThread(int thread_id)
{
    m_current_thread = thread_id;
}

void Debugger_GDB_MI::RunQueue()
{
    if(m_executor.IsRunning())
    {
        Notifications notifications(this, m_executor, m_page_index, false);
        dbg_mi::DispatchResults(m_executor, m_actions, notifications);

        if(m_executor.IsStopped())
            m_actions.Run(m_executor);
    }
}

void Debugger_GDB_MI::ParseOutput(wxString const &str)
{
    if(!str.IsEmpty())
    {
        wxArrayString const &lines = GetArrayFromString(str, _T('\n'));
        for(size_t ii = 0; ii < lines.GetCount(); ++ii)
            m_executor.ProcessOutput(lines[ii]);
        m_actions.Run(m_executor);
    }
}

struct StopNotification
{
    StopNotification(cbDebuggerPlugin *plugin, dbg_mi::GDBExecutor &executor) :
        m_plugin(plugin),
        m_executor(executor)
    {
    }

    void operator()(bool stopped)
    {
        m_executor.Stopped(stopped);
        if(!stopped)
        {
            m_plugin->ClearActiveMarkFromAllEditors();
        }
    }

    cbDebuggerPlugin *m_plugin;
    dbg_mi::GDBExecutor &m_executor;
};

int Debugger_GDB_MI::Debug(bool breakOnEntry)
{
    ShowLog();
    Manager::Get()->GetLogManager()->Log(_T("start debugger"), m_page_index);

    ProjectManager &project_manager = *Manager::Get()->GetProjectManager();
    cbProject *project = project_manager.GetActiveProject();
    if(!project)
    {
        Manager::Get()->GetLogManager()->LogError(_T("no active project"), m_page_index);
        return 1;
    }
    Compiler *compiler;
    ProjectBuildTarget *target;
    SelectCompiler(*project, compiler, target, 0);

    if(!compiler)
    {
        Manager::Get()->GetLogManager()->LogError(_T("no compiler found!"), m_page_index);
        return 2;
    }
    if(!target)
    {
        Manager::Get()->GetLogManager()->LogError(_T("no target found!"), m_page_index);
        return 3;
    }

    // is gdb accessible, i.e. can we find it?
    wxString debugger;
    debugger = compiler->GetPrograms().DBG;
    debugger.Trim();
    debugger.Trim(true);
    if(debugger.IsEmpty())
    {
        Manager::Get()->GetLogManager()->LogError(_T("no debugger executable found!"), m_page_index);
        return 4;
    }
    // access the gdb executable name
    debugger = FindDebuggerExecutable(compiler);
    if(debugger.IsEmpty())
    {
        Manager::Get()->GetLogManager()->LogError(_T("no debugger executable found (full path)!"), m_page_index);
        return 5;
    }

    wxString debuggee = GetDebuggee(target);

    Manager::Get()->GetLogManager()->Log(_T("GDB path: ") + debugger, m_page_index);
    Manager::Get()->GetLogManager()->Log(_T("DEBUGGEE path: ") + debuggee, m_page_index);

    wxString cmd;
    cmd << debugger;
    cmd << _T(" -nx");          // don't run .gdbinit
    cmd << _T(" -fullname ");   // report full-path filenames when breaking
    cmd << _T(" -quiet");       // don't display version on startup
    cmd << _T(" --interpreter=mi");
    cmd << _T(" -args ") << debuggee;

    // start the gdb process
    wxString working_dir = project->GetBasePath();
    Manager::Get()->GetLogManager()->Log(_T("Command-line: ") + cmd, m_page_index);
    Manager::Get()->GetLogManager()->Log(_T("Working dir : ") + working_dir, m_page_index);

    int ret = m_executor.LaunchProcess(cmd, working_dir, id_gdb_process, this);

    m_executor.Stopped(true);
    m_executor.Execute(_T("-enable-timings"));
    CommitBreakpoints(true);

    CommitRunCommand(wxT("-exec-run"));
    m_actions.Run(m_executor);

    m_timer_poll_debugger.Start(20);
    return 0;
}

void Debugger_GDB_MI::CommitBreakpoints(bool force)
{
    Manager::Get()->GetLogManager()->Log(wxT("Debugger_GDB_MI::CommitBreakpoints"), m_dbg_page_index);
    for(Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        // FIXME (obfuscated#): pointers inside the vector can be dangerous!!!
        if((*it)->GetIndex() == -1 || force)
        {
            m_actions.Add(new dbg_mi::BreakpointAddAction(*it, m_execution_logger));
        }
    }

    for(Breakpoints::const_iterator it = m_temporary_breakpoints.begin(); it != m_temporary_breakpoints.end(); ++it)
    {
        AddStringCommand(wxString::Format(_T("-break-insert -t %s:%d"), (*it)->Get().GetFilename().c_str(),
                                          (*it)->Get().GetLine()));
    }
    m_temporary_breakpoints.clear();
}

void Debugger_GDB_MI::CommitRunCommand(wxString const &command)
{
    m_actions.Add(new dbg_mi::RunAction<StopNotification>(this, command,
                                                          StopNotification(this, m_executor),
                                                          m_execution_logger)
                  );
}

void Debugger_GDB_MI::RunToCursor(const wxString& filename, int line, const wxString& line_text)
{
    if(IsRunning())
    {
        if(IsStopped())
        {
            CommitRunCommand(wxString::Format(wxT("-exec-until %s:%d"), filename.c_str(), line));
        }
    }
    else
    {
        dbg_mi::Breakpoint::Pointer ptr(new dbg_mi::Breakpoint(cbBreakpoint(filename, line)));
        m_temporary_breakpoints.push_back(ptr);
        Debug(false);
    }
}

void Debugger_GDB_MI::Continue()
{
    if(!IsStopped() && !m_executor.Interupting())
    {
        dbg_mi::Logger *log = m_executor.GetLogger();
        if(log)
            log->Debug(wxT("Continue failed -> debugger is not interupted!"));

        return;
    }
    Manager::Get()->GetLogManager()->Log(wxT("Debugger_GDB_MI::Continue"), m_dbg_page_index);
    CommitRunCommand(wxT("-exec-continue"));
}

void Debugger_GDB_MI::Next()
{
    CommitRunCommand(wxT("-exec-next"));
}

void Debugger_GDB_MI::NextInstruction()
{
    CommitRunCommand(wxT("-exec-next-instruction"));
}

void Debugger_GDB_MI::Step()
{
    CommitRunCommand(wxT("-exec-step"));
}

void Debugger_GDB_MI::StepOut()
{
    CommitRunCommand(wxT("-exec-finish"));
}

void Debugger_GDB_MI::Break()
{
    m_executor.Interupt(false);
}

void Debugger_GDB_MI::Stop()
{
    if(!IsRunning())
        return;
    ClearActiveMarkFromAllEditors();
    Manager::Get()->GetLogManager()->Log(_T("stop debugger"), m_page_index);
    m_executor.ForceStop();
}
bool Debugger_GDB_MI::IsRunning() const
{
    return m_executor.IsRunning();
}
bool Debugger_GDB_MI::IsStopped() const
{
    return m_executor.IsStopped();
}
int Debugger_GDB_MI::GetExitCode() const
{
    #warning "not implemented"
    return 0;
}

int Debugger_GDB_MI::GetStackFrameCount() const
{
    return m_backtrace.size();
}

const cbStackFrame& Debugger_GDB_MI::GetStackFrame(int index) const
{
    return m_backtrace[index];
}

void Debugger_GDB_MI::SwitchToFrame(int number)
{
//    if(IsRunning() && IsStopped())
//    {
//        if(number < static_cast<int>(m_backtrace.size()))
//        {
//            int frame = m_backtrace[number].GetNumber();
//            AddStringCommand(wxString::Format(wxT("-stack-select-frame %d"), frame));
//        }
//    }
    #warning "not implemented"
}

cbBreakpoint* Debugger_GDB_MI::AddBreakpoint(const wxString& filename, int line)
{
    if(!IsStopped())
    {
        Manager::Get()->GetLogManager()->Log(wxString::Format(wxT("Debugger_GDB_MI::Addbreakpoint: %s:%d"),
                                                              filename.c_str(), line),
                                             m_dbg_page_index);
        m_executor.Interupt();

        dbg_mi::Breakpoint::Pointer ptr(new dbg_mi::Breakpoint(cbBreakpoint(filename, line)));
        m_breakpoints.push_back(ptr);
        CommitBreakpoints(false);
        Continue();
    }
    else
    {
        dbg_mi::Breakpoint::Pointer ptr(new dbg_mi::Breakpoint(cbBreakpoint(filename, line)));
        m_breakpoints.push_back(ptr);
        if(IsRunning())
            CommitBreakpoints(false);
    }

    return &m_breakpoints.back()->Get();
}

cbBreakpoint* Debugger_GDB_MI::AddDataBreakpoint(const wxString& dataExpression)
{
    #warning "not implemented"
    return NULL;
}

int Debugger_GDB_MI::GetBreakpointsCount() const
{
    return m_breakpoints.size();
}

cbBreakpoint* Debugger_GDB_MI::GetBreakpoint(int index)
{
    return &m_breakpoints[index]->Get();
}

const cbBreakpoint* Debugger_GDB_MI::GetBreakpoint(int index) const
{
    return &m_breakpoints[index]->Get();
}

void Debugger_GDB_MI::UpdateBreakpoint(cbBreakpoint *breakpoint)
{
    for(Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        dbg_mi::Breakpoint &current = **it;
        if(&current.Get() != breakpoint)
            continue;

        switch(breakpoint->GetType())
        {
        case cbBreakpoint::Code:
            {
                cbBreakpoint temp(*breakpoint);
                EditBreakpointDlg dialog(&temp);
                PlaceWindow(&dialog);
                if(dialog.ShowModal() == wxID_OK)
                {
                    // if the breakpoint is not sent to debugger, just copy
                    if(current.GetIndex() != -1 || !IsRunning())
                    {
                        current.Get() = temp;
                        current.SetIndex(-1);
                    }
                    else
                    {
//                        bool resume = !m_executor.IsStopped();
//                        if(resume)
//                            m_executor.Interupt();
#warning "not implemented"
                        bool changed = false;
                        if(breakpoint->IsEnabled() == temp.IsEnabled())
                        {
                            if(breakpoint->IsEnabled())
                                AddStringCommand(wxString::Format(wxT("-break-enable %d"), current.GetIndex()));
                            else
                                AddStringCommand(wxString::Format(wxT("-break-disable %d"), current.GetIndex()));
                            changed = true;
                        }

                        if(changed)
                        {
                            m_executor.Interupt();
                            Continue();
                        }
                    }

                }
            }
            break;
        case cbBreakpoint::Data:
#warning "not implemented"
            break;
        }
    }
}

void Debugger_GDB_MI::DeleteBreakpoint(cbBreakpoint *breakpoint)
{
    for(Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        dbg_mi::Breakpoint &current = **it;
        if(&current.Get() == breakpoint)
        {
            Manager::Get()->GetLogManager()->Log(wxString::Format(wxT("Debugger_GDB_MI::DeleteBreakpoint: %s:%d"),
                                                                  breakpoint->GetFilename().c_str(),
                                                                  breakpoint->GetLine()),
                                                 m_dbg_page_index);
            if(current.GetIndex() != -1)
            {
                if(!IsStopped())
                {
                    m_executor.Interupt();
                    AddStringCommand(wxString::Format(wxT("-break-delete %d"), current.GetIndex()));
                    Continue();
                }
                else
                    AddStringCommand(wxString::Format(wxT("-break-delete %d"), current.GetIndex()));
            }
            m_breakpoints.erase(it);
            return;
        }
    }
}

void Debugger_GDB_MI::DeleteAllBreakpoints()
{
    if(IsRunning())
    {
        wxString breaklist;
        for(Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
        {
            dbg_mi::Breakpoint &current = **it;
            if(current.GetIndex() != -1)
                breaklist += wxString::Format(wxT(" %d"), current.GetIndex());
        }

        if(!breaklist.empty())
        {
            if(!IsStopped())
            {
                m_executor.Interupt();
                AddStringCommand(wxT("-break-delete") + breaklist);
                Continue();
            }
            else
                AddStringCommand(wxT("-break-delete") + breaklist);
        }
    }
    m_breakpoints.clear();
}

void Debugger_GDB_MI::ShiftBreakpoint(int index, int lines_to_shift)
{
    #warning "not implemented"
}

int Debugger_GDB_MI::GetThreadsCount() const
{
    return m_threads.size();
}

const cbThread& Debugger_GDB_MI::GetThread(int index) const
{
    return m_threads[index];
}

bool Debugger_GDB_MI::SwitchToThread(int thread_number)
{
    if(IsStopped())
    {
        dbg_mi::SwitchToThread<Notifications> *a;
        a = new dbg_mi::SwitchToThread<Notifications>(thread_number, m_execution_logger,
                                                      Notifications(this, m_executor,
                                                                    m_dbg_page_index,
                                                                    true)
                                                      );
        m_actions.Add(a);
        return true;
    }
    else
        return false;
}

cbWatch* Debugger_GDB_MI::AddWatch(const wxString& symbol)
{
    #warning "not implemented"
    return NULL;
}

void Debugger_GDB_MI::DeleteWatch(cbWatch *watch)
{
    #warning "not implemented"
}

bool Debugger_GDB_MI::HasWatch(cbWatch *watch)
{
    #warning "not implemented"
    return false;
}

void Debugger_GDB_MI::ShowWatchProperties(cbWatch *watch)
{
    #warning "not implemented"
}

bool Debugger_GDB_MI::SetWatchValue(cbWatch *watch, const wxString &value)
{
    #warning "not implemented"
    return false;
}

void Debugger_GDB_MI::SendCommand(const wxString& cmd)
{
    LogManager *log = Manager::Get()->GetLogManager();
    if(!IsRunning())
    {
        wxString message(wxT("Command will not be executed because the debugger is not running!"));
        log->Log(message, m_page_index);
        log->Log(message, m_dbg_page_index);
        return;
    }
    if(!IsStopped())
    {
        wxString message(wxT("Command will not be executed because the debugger/debuggee is not paused/interupted!"));
        log->Log(message, m_page_index);
        log->Log(message, m_dbg_page_index);
        return;
    }

    m_executor.Execute(cmd);
}

void Debugger_GDB_MI::AttachToProcess(const wxString& pid)
{
    #warning "not implemented"
}

void Debugger_GDB_MI::DetachFromProcess()
{
    #warning "not implemented"
}

void Debugger_GDB_MI::RequestUpdate(DebugWindows window)
{
    switch(window)
    {
    case Backtrace:
        if(IsStopped())
            m_actions.Add(new dbg_mi::GenerateBacktrace(m_backtrace, m_execution_logger));
        break;

    case Threads:
        if(IsStopped())
            m_actions.Add(new dbg_mi::GenerateThreadsList(m_threads, m_current_thread, m_execution_logger));
        break;
    }
}

void Debugger_GDB_MI::GetCurrentPosition(wxString &filename, int &line)
{
    #warning "not implemented"
}


