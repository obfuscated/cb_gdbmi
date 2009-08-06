#include <sdk.h> // Code::Blocks SDK

#include "Debugger_GDB_MI.h"

#include <algorithm>
#include <wx/xrc/xmlres.h>

#include <cbproject.h>
#include <compilerfactory.h>
#include <configurationpanel.h>
#include <configmanager.h>
#include <loggers.h>
#include <logmanager.h>
#include <macrosmanager.h>
#include <pipedprocess.h>
#include <projectmanager.h>

#include "actions.h"
#include "cmd_result_parser.h"



// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<Debugger_GDB_MI> reg(_T("debugger_gdbmi"));
}

int id_gdb_process = wxNewId();
int id_gdb_poll_timer = wxNewId();

// events handling
BEGIN_EVENT_TABLE(Debugger_GDB_MI, cbDebuggerPlugin)

    EVT_PIPEDPROCESS_STDOUT(id_gdb_process, Debugger_GDB_MI::OnGDBOutput)
    EVT_PIPEDPROCESS_STDERR(id_gdb_process, Debugger_GDB_MI::OnGDBError)
    EVT_PIPEDPROCESS_TERMINATED(id_gdb_process, Debugger_GDB_MI::OnGDBTerminated)

    EVT_IDLE(Debugger_GDB_MI::OnIdle)
    EVT_TIMER(id_gdb_poll_timer, Debugger_GDB_MI::OnTimer)

    EVT_DBGMI_NOTIFICATION(Debugger_GDB_MI::OnGDBNotification)

END_EVENT_TABLE()

// constructor
Debugger_GDB_MI::Debugger_GDB_MI() :
    m_menu(NULL),
    m_process(NULL),
    m_log(NULL)
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if(!Manager::LoadResource(_T("Debugger_GDB_MI.zip")))
    {
        NotifyMissingFile(_T("Debugger_GDB_MI.zip"));
    }
}

// destructor
Debugger_GDB_MI::~Debugger_GDB_MI()
{
}

void Debugger_GDB_MI::OnAttach()
{
    m_timer_poll_debugger.SetOwner(this, id_gdb_poll_timer);

    Manager::Get()->GetDebuggerManager()->RegisterDebugger(this, _T("gdbmi"));

    LogManager &message_manager = *Manager::Get()->GetLogManager();
    if(!m_log)
        m_log = new TextCtrlLogger(true);
    m_page_index = message_manager.SetLog(m_log);
    LogSlot &slot = message_manager.Slot(m_page_index);

    m_command_queue.SetDebugLogPageIndex(m_page_index);

    // set log image
    wxString prefix = ConfigManager::GetDataFolder() + _T("/images/");
    wxBitmap* bmp = new wxBitmap(cbLoadBitmap(prefix + _T("misc_16x16.png"), wxBITMAP_TYPE_PNG));

    slot.title = _("Debugger MI");
    slot.icon = bmp;

    CodeBlocksLogEvent evtAdd(cbEVT_ADD_LOG_WINDOW, m_log, slot.title, slot.icon);
    Manager::Get()->ProcessEvent(evtAdd);

    // do whatever initialization you need for your plugin
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be TRUE...
    // You should check for it in other functions, because if it
    // is FALSE, it means that the application did *not* "load"
    // (see: does not need) this plugin...
}

void Debugger_GDB_MI::OnRelease(bool appShutDown)
{
    if(Manager::Get()->GetLogManager())
    {
        CodeBlocksLogEvent evt(cbEVT_REMOVE_LOG_WINDOW, m_log);
        Manager::Get()->ProcessEvent(evt);
        m_log = NULL;
    }

    Manager::Get()->GetDebuggerManager()->UnregisterDebugger(this);
    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...
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

void Debugger_GDB_MI::BuildMenu(wxMenuBar* menuBar)
{
    Manager::Get()->GetLogManager()->LogError(_T("test"));
    m_menu = wxXmlResource::Get()->LoadMenu(_T("debugger_gdbmi_menu"));

    if(!m_menu)
    {
        Manager::Get()->GetLogManager()->LogError(_T("menu creation failed :("));
        return;
    }

    // ok, now, where do we insert?
    // three possibilities here:
    // a) locate "Compile" menu and insert after it
    // b) locate "Project" menu and insert after it
    // c) if not found (?), insert at pos 5
    int finalPos = 5;
    int projcompMenuPos = menuBar->FindMenu(_("&Debug"));
    if (projcompMenuPos == wxNOT_FOUND)
        projcompMenuPos = menuBar->FindMenu(_("&Build"));
    if (projcompMenuPos == wxNOT_FOUND)
        projcompMenuPos = menuBar->FindMenu(_("&Compile"));

    if (projcompMenuPos != wxNOT_FOUND)
        finalPos = projcompMenuPos + 1;
    else
    {
        projcompMenuPos = menuBar->FindMenu(_("&Project"));
        if (projcompMenuPos != wxNOT_FOUND)
            finalPos = projcompMenuPos + 1;
    }
    menuBar->Insert(finalPos, m_menu, _("&DebugMI"));
}

void Debugger_GDB_MI::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
{
    //Some library module is ready to display a pop-up menu.
    //Check the parameter \"type\" and see which module it is
    //and append any items you need in the menu...
    //TIP: for consistency, add a separator as the first item...
    NotImplemented(_T("Debugger_GDB_MI::BuildModuleMenu()"));
}

bool Debugger_GDB_MI::BuildToolBar(wxToolBar* toolBar)
{
    //The application is offering its toolbar for your plugin,
    //to add any toolbar items you want...
    //Append any items you need on the toolbar...
    NotImplemented(_T("Debugger_GDB_MI::BuildToolBar()"));

    // return true if you add toolbar items
    return false;
}

void Debugger_GDB_MI::ShowLog()
{
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

int Debugger_GDB_MI::LaunchProcess(const wxString& cmd, const wxString& cwd)
{
    if(m_process)
        return -1;

    // start the gdb process
    m_process = new PipedProcess((void**)&m_process, this, id_gdb_process, true, cwd);
    Manager::Get()->GetLogManager()->Log(_("Starting debugger: "), m_page_index);
    m_pid = wxExecute(cmd, wxEXEC_ASYNC, m_process);

#ifdef __WXMAC__
    if (m_pid == -1)
    {
        Manager::Get()->GetLogManager()->LogError(_("debugger has fake macos PID"), m_page_index);
    }
#endif

    if (!m_pid)
    {
        delete m_process;
        m_process = 0;
        Manager::Get()->GetLogManager()->Log(_("failed"), m_page_index);
        return -1;
    }
    else if (!m_process->GetOutputStream())
    {
        delete m_process;
        m_process = 0;
        Manager::Get()->GetLogManager()->Log(_("failed (to get debugger's stdin)"), m_page_index);
        return -2;
    }
    else if (!m_process->GetInputStream())
    {
        delete m_process;
        m_process = 0;
        Manager::Get()->GetLogManager()->Log(_("failed (to get debugger's stdout)"), m_page_index);
        return -2;
    }
    else if (!m_process->GetErrorStream())
    {
        delete m_process;
        m_process = 0;
        Manager::Get()->GetLogManager()->Log(_("failed (to get debugger's stderr)"), m_page_index);
        return -2;
    }
    Manager::Get()->GetLogManager()->Log(_("done"), m_page_index);

    return 0;
}

void Debugger_GDB_MI::OnGDBOutput(wxCommandEvent& event)
{
    wxString msg = event.GetString();
    if (!msg.IsEmpty())
    {
//        Manager::Get()->GetLogManager()->Log(m_PageIndex, _T("O>>> %s"), msg.c_str());
        ParseOutput(msg);
    }
}

void Debugger_GDB_MI::OnGDBError(wxCommandEvent& event)
{
    wxString msg = event.GetString();
    if (!msg.IsEmpty())
    {
//        Manager::Get()->GetLogManager()->Log(m_PageIndex, _T("E>>> %s"), msg.c_str());
        ParseOutput(msg);
    }
}

void Debugger_GDB_MI::OnGDBTerminated(wxCommandEvent& event)
{
    Manager::Get()->GetLogManager()->Log(_T("debugger terminated!"), m_page_index);
    m_timer_poll_debugger.Stop();
}

void Debugger_GDB_MI::OnIdle(wxIdleEvent& event)
{
    if (m_process && m_process->HasInput())
        event.RequestMore();
    else
        event.Skip();
}

void Debugger_GDB_MI::OnTimer(wxTimerEvent& event)
{
    RunQueue();
    wxWakeUpIdle();
}

void Debugger_GDB_MI::OnGDBNotification(dbg_mi::NotificationEvent &event)
{
    wxString const &log_str = event.GetResultParser()->MakeDebugString();
    Manager::Get()->GetLogManager()->Log(log_str, m_page_index);

    if(emit_watch)
    {
        Manager::Get()->GetLogManager()->Log(_T("notification event recieved!"), m_page_index);
        emit_watch = false;

        m_command_queue.AddAction(new dbg_mi::WatchAction(_T("v"), m_page_index));
        m_command_queue.AddAction(new dbg_mi::WatchAction(_T("fill_vector"), m_page_index));
    }
}

void Debugger_GDB_MI::AddStringCommand(wxString const &command)
{
    dbg_mi::Command *cmd = new dbg_mi::Command();
    cmd->SetString(command);
    m_command_queue.AddCommand(cmd, true);
}

void Debugger_GDB_MI::RunQueue()
{
    if(m_process)
        m_command_queue.RunQueue(m_process);
}

void Debugger_GDB_MI::ParseOutput(wxString const &str)
{
    if(str.IsEmpty())
        return;
    Manager::Get()->GetLogManager()->Log(_T(">>>") + str, m_page_index);
    m_command_queue.AccumulateOutput(str);
}


bool Debugger_GDB_MI::AddBreakpoint(const wxString& file, int line)
{
    m_breakpoints.push_back(cbBreakpoint(file, line));
    return true;
}
bool Debugger_GDB_MI::AddBreakpoint(const wxString& functionSignature)
{
    return false;
}


struct MatchFileLine
{
    MatchFileLine(const wxString &file, int line) : m_file(file), m_line(line) {}

    bool operator()(const cbBreakpoint &b) const
    {
        return b.GetFilename() == m_file && b.GetLine() == m_line;
    }

    const wxString& m_file;
    int m_line;
};

bool Debugger_GDB_MI::RemoveBreakpoint(const wxString& file, int line)
{
    Breakpoints::iterator last = std::remove_if(m_breakpoints.begin(), m_breakpoints.end(), MatchFileLine(file, line));
    m_breakpoints.erase(last, m_breakpoints.end());
    return true;
}
bool Debugger_GDB_MI::RemoveBreakpoint(const wxString& functionSignature)
{
    return false;
}

struct MatchFile
{
    MatchFile(const wxString &file) : m_file(file) {}

    bool operator()(const cbBreakpoint &b) const
    {
        return b.GetFilename() == m_file;
    }

    const wxString& m_file;
};

bool Debugger_GDB_MI::RemoveAllBreakpoints(const wxString& file)
{
    if(file == wxEmptyString)
    {
        m_breakpoints.clear();
        return true;
    }
    else
    {
        Breakpoints::iterator last = std::remove_if(m_breakpoints.begin(), m_breakpoints.end(), MatchFile(file));
        m_breakpoints.erase(last, m_breakpoints.end());
        return true;
    }
}
void Debugger_GDB_MI::EditorLinesAddedOrRemoved(cbEditor* editor, int startline, int lines)
{
}
int Debugger_GDB_MI::GetBreakpointsCount() const
{
    return m_breakpoints.size();
}
void Debugger_GDB_MI::GetBreakpoint(int index, cbBreakpoint& breakpoint) const
{
    breakpoint = m_breakpoints[index];
}
void Debugger_GDB_MI::UpdateBreakpoint(int index, cbBreakpoint const &breakpoint)
{
    m_breakpoints[index] = breakpoint;
}
int Debugger_GDB_MI::Debug()
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

    emit_watch = true;

    int ret = LaunchProcess(cmd, working_dir);

    AddStringCommand(_T("-enable-timings"));
//    AddStringCommand(_T("-break-insert main.cpp:55"));
    for(Breakpoints::const_iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        AddStringCommand(wxString::Format(_T("-break-insert %s:%d"), it->GetFilename().c_str(), it->GetLine()));
    }
    AddStringCommand(_T("-exec-run"));

    m_timer_poll_debugger.Start(20);
    return 0;
}
void Debugger_GDB_MI::Continue()
{
}
void Debugger_GDB_MI::Next()
{
    AddStringCommand(_T("-exec-next"));
}
void Debugger_GDB_MI::NextInstruction()
{
    AddStringCommand(_T("-exec-next-instruction"));
}
void Debugger_GDB_MI::Step()
{
    AddStringCommand(_T("-exec-step"));
}
void Debugger_GDB_MI::StepOut()
{
    AddStringCommand(_T("-exec-return"));
}
void Debugger_GDB_MI::Break()
{
}
void Debugger_GDB_MI::Stop()
{
    Manager::Get()->GetLogManager()->Log(_T("stop debugger"), m_page_index);
    if(m_process)
        AddStringCommand(_T("-gdb-exit"));
}
bool Debugger_GDB_MI::IsRunning() const
{
    return m_process;
}
bool Debugger_GDB_MI::IsStopped() const
{
    return true;
}
int Debugger_GDB_MI::GetExitCode() const
{
    return 0;
}
